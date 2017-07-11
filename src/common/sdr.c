/* SDR processing
 *
 * (C) 2017 by Andreas Eversberg <jolly@eversberg.eu>
 * All Rights Reserved
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "sample.h"
#include "iir_filter.h"
#include "fm_modulation.h"
#include "sender.h"
#ifdef HAVE_UHD
#include "uhd.h"
#endif
#ifdef HAVE_SOAPY
#include "soapy.h"
#endif
#include "debug.h"

typedef struct sdr_chan {
	double		tx_frequency;	/* frequency used */
	double		rx_frequency;	/* frequency used */
	fm_mod_t	mod;		/* modulator instance */
	fm_demod_t	demod;		/* demodulator instance */
} sdr_chan_t;

typedef struct sdr {
	sdr_chan_t	*chan;		/* settings for all channels */
	int		paging_channel;	/* if set, points to paging channel */
	sdr_chan_t	paging_chan;	/* settings for extra paging channel */
	int		channels;	/* number of frequencies */
	double		samplerate;	/* IQ rate */
	double		amplitude;	/* amplitude of each carrier */
	wave_rec_t	wave_rx_rec;
	wave_rec_t	wave_tx_rec;
	wave_play_t	wave_rx_play;
	wave_play_t	wave_tx_play;
} sdr_t;

static int sdr_use_uhd, sdr_use_soapy;
static int sdr_channel;
static const char *sdr_device_args, *sdr_stream_args, *sdr_tune_args;
static const char *sdr_rx_antenna, *sdr_tx_antenna;
static double sdr_rx_gain, sdr_tx_gain;
const char *sdr_write_iq_rx_wave, *sdr_write_iq_tx_wave, *sdr_read_iq_rx_wave, *sdr_read_iq_tx_wave;
static double sdr_bandwidth;

int sdr_init(int sdr_uhd, int sdr_soapy, int channel, const char *device_args, const char *stream_args, const char *tune_args, const char *tx_antenna, const char *rx_antenna, double tx_gain, double rx_gain, double bandwidth, const char *write_iq_tx_wave, const char *write_iq_rx_wave, const char *read_iq_tx_wave, const char *read_iq_rx_wave)
{
	sdr_use_uhd = sdr_uhd;
	sdr_use_soapy = sdr_soapy;
	sdr_channel = channel;
	sdr_device_args = strdup(device_args);
	sdr_stream_args = strdup(stream_args);
	sdr_tune_args = strdup(tune_args);
	sdr_tx_antenna = strdup(tx_antenna);
	sdr_rx_antenna = strdup(rx_antenna);
	sdr_tx_gain = tx_gain;
	sdr_rx_gain = rx_gain;
	sdr_bandwidth = bandwidth;
	sdr_write_iq_tx_wave = write_iq_tx_wave;
	sdr_write_iq_rx_wave = write_iq_rx_wave;
	sdr_read_iq_tx_wave = read_iq_tx_wave;
	sdr_read_iq_rx_wave = read_iq_rx_wave;

	return 0;
}

void *sdr_open(const char __attribute__((__unused__)) *audiodev, double *tx_frequency, double *rx_frequency, int channels, double paging_frequency, int samplerate, double max_deviation, double max_modulation)
{
	sdr_t *sdr;
	double bandwidth;
	double tx_center_frequency = 0.0, rx_center_frequency = 0.0;
	int rc;
	int c;

	display_iq_init(samplerate);
	display_spectrum_init(samplerate);

	bandwidth = 2.0 * (max_deviation + max_modulation);
	PDEBUG(DSDR, DEBUG_INFO, "Require bandwidth of 2 * (%.1f + %.1f) = %.1f\n", max_deviation / 1000, max_modulation / 1000, bandwidth / 1000);

	if (channels < 1) {
		PDEBUG(DSDR, DEBUG_ERROR, "No channel given, please fix!\n");
		abort();
	}

	sdr = calloc(sizeof(*sdr), 1);
	if (!sdr) {
		PDEBUG(DSDR, DEBUG_ERROR, "NO MEM!\n");
		goto error;
	}
	sdr->channels = channels;
	sdr->samplerate = samplerate;
	sdr->amplitude = 1.0 / (double)channels;

	/* special case where we use a paging frequency */
	if (paging_frequency) {
		/* add extra paging channel */
		sdr->paging_channel = channels;
	}

	/* create list of channel states */
	sdr->chan = calloc(sizeof(*sdr->chan), channels + (sdr->paging_channel != 0));
	if (!sdr->chan) {
		PDEBUG(DSDR, DEBUG_ERROR, "NO MEM!\n");
		goto error;
	}

	if (tx_frequency) {
		/* calculate required bandwidth (IQ rate) */
		for (c = 0; c < channels; c++) {
			PDEBUG(DSDR, DEBUG_INFO, "Frequency #%d: TX = %.6f MHz\n", c, tx_frequency[c] / 1e6);
			sdr->chan[c].tx_frequency = tx_frequency[c];
		}
		if (sdr->paging_channel) {
			PDEBUG(DSDR, DEBUG_INFO, "Paging Frequency: TX = %.6f MHz\n", paging_frequency / 1e6);
			sdr->chan[sdr->paging_channel].tx_frequency = paging_frequency;
		}

		double tx_low_frequency = sdr->chan[0].tx_frequency, tx_high_frequency = sdr->chan[0].tx_frequency;
		for (c = 1; c < channels; c++) {
			if (sdr->chan[c].tx_frequency < tx_low_frequency)
				tx_low_frequency = sdr->chan[c].tx_frequency;
			if (sdr->chan[c].tx_frequency > tx_high_frequency)
				tx_high_frequency = sdr->chan[c].tx_frequency;
		}
		if (sdr->paging_channel) {
			if (sdr->chan[sdr->paging_channel].tx_frequency < tx_low_frequency)
				tx_low_frequency = sdr->chan[sdr->paging_channel].tx_frequency;
			if (sdr->chan[sdr->paging_channel].tx_frequency > tx_high_frequency)
				tx_high_frequency = sdr->chan[sdr->paging_channel].tx_frequency;
		}
		/* range of TX */
		double range = tx_high_frequency - tx_low_frequency;
		if (range)
			PDEBUG(DSDR, DEBUG_DEBUG, "Range between all TX Frequencies: %.6f MHz\n", range / 1e6);
		if (range * 2 > sdr->samplerate) {
			// why that? actually i don't know. i just want to be safe....
			PDEBUG(DSDR, DEBUG_NOTICE, "The sample rate must be at least twice the range between frequencies.\n");
			PDEBUG(DSDR, DEBUG_NOTICE, "The given rate is %.6f MHz, but required rate must be >= %.6f MHz\n", sdr->samplerate / 1e6, range * 2.0 / 1e6);
			PDEBUG(DSDR, DEBUG_NOTICE, "Please increase samplerate!\n");
			goto error;
		}
		tx_center_frequency = (tx_high_frequency + tx_low_frequency) / 2.0;
		PDEBUG(DSDR, DEBUG_INFO, "Using center frequency: TX %.6f MHz\n", tx_center_frequency / 1e6);
		/* set offsets to center frequency */
		for (c = 0; c < channels; c++) {
			double tx_offset;
			tx_offset = sdr->chan[c].tx_frequency - tx_center_frequency;
			PDEBUG(DSDR, DEBUG_DEBUG, "Frequency #%d: TX offset: %.6f MHz\n", c, tx_offset / 1e6);
			fm_mod_init(&sdr->chan[c].mod, sdr->samplerate, tx_offset, sdr->amplitude);
		}
		if (sdr->paging_channel) {
			double tx_offset;
			tx_offset = sdr->chan[sdr->paging_channel].tx_frequency - tx_center_frequency;
			PDEBUG(DSDR, DEBUG_DEBUG, "Paging Frequency: TX offset: %.6f MHz\n", tx_offset / 1e6);
			fm_mod_init(&sdr->chan[sdr->paging_channel].mod, sdr->samplerate, tx_offset, sdr->amplitude);
		}
		/* show gain */
		PDEBUG(DSDR, DEBUG_INFO, "Using gain: TX %.1f dB\n", sdr_tx_gain);
		/* open wave */
		if (sdr_write_iq_tx_wave) {
			rc = wave_create_record(&sdr->wave_tx_rec, sdr_write_iq_tx_wave, sdr->samplerate, 2, 1.0);
			if (rc < 0) {
				PDEBUG(DSDR, DEBUG_ERROR, "Failed to create WAVE recoding instance!\n");
				goto error;
			}
		}
		if (sdr_read_iq_tx_wave) {
			rc = wave_create_playback(&sdr->wave_tx_play, sdr_read_iq_tx_wave, sdr->samplerate, 2, 1.0);
			if (rc < 0) {
				PDEBUG(DSDR, DEBUG_ERROR, "Failed to create WAVE playback instance!\n");
				goto error;
			}
		}
	}

	if (rx_frequency) {
		for (c = 0; c < channels; c++) {
			PDEBUG(DSDR, DEBUG_INFO, "Frequency #%d: RX = %.6f MHz\n", c, rx_frequency[c] / 1e6);
			sdr->chan[c].rx_frequency = rx_frequency[c];
		}

		/* calculate required bandwidth (IQ rate) */
		double rx_low_frequency = sdr->chan[0].rx_frequency, rx_high_frequency = sdr->chan[0].rx_frequency;
		for (c = 1; c < channels; c++) {
			if (sdr->chan[c].rx_frequency < rx_low_frequency)
				rx_low_frequency = sdr->chan[c].rx_frequency;
			if (sdr->chan[c].rx_frequency > rx_high_frequency)
				rx_high_frequency = sdr->chan[c].rx_frequency;
		}
		/* range of RX */
		double range = rx_high_frequency - rx_low_frequency;
		if (range)
			PDEBUG(DSDR, DEBUG_DEBUG, "Range between all RX Frequencies: %.6f MHz\n", range / 1e6);
		if (range * 2.0 > sdr->samplerate) {
			// why that? actually i don't know. i just want to be safe....
			PDEBUG(DSDR, DEBUG_NOTICE, "The sample rate must be at least twice the range between frequencies. Please increment samplerate!\n");
			goto error;
		}
		rx_center_frequency = (rx_high_frequency + rx_low_frequency) / 2.0;
		PDEBUG(DSDR, DEBUG_INFO, "Using center frequency: RX %.6f MHz\n", rx_center_frequency / 1e6);
		/* set offsets to center frequency */
		for (c = 0; c < channels; c++) {
			double rx_offset;
			rx_offset = sdr->chan[c].rx_frequency - rx_center_frequency;
			PDEBUG(DSDR, DEBUG_DEBUG, "Frequency #%d: RX offset: %.6f MHz\n", c, rx_offset / 1e6);
			fm_demod_init(&sdr->chan[c].demod, sdr->samplerate, rx_offset, bandwidth);
		}
		/* show gain */
		PDEBUG(DSDR, DEBUG_INFO, "Using gain: RX %.1f dB\n", sdr_rx_gain);
		/* open wave */
		if (sdr_write_iq_rx_wave) {
			rc = wave_create_record(&sdr->wave_rx_rec, sdr_write_iq_rx_wave, sdr->samplerate, 2, 1.0);
			if (rc < 0) {
				PDEBUG(DSDR, DEBUG_ERROR, "Failed to create WAVE recoding instance!\n");
				goto error;
			}
		}
		if (sdr_read_iq_rx_wave) {
			rc = wave_create_playback(&sdr->wave_rx_play, sdr_read_iq_rx_wave, sdr->samplerate, 2, 1.0);
			if (rc < 0) {
				PDEBUG(DSDR, DEBUG_ERROR, "Failed to create WAVE playback instance!\n");
				goto error;
			}
		}
	}

#ifdef HAVE_UHD
	if (sdr_use_uhd) {
		rc = uhd_open(sdr_channel, sdr_device_args, sdr_stream_args, sdr_tune_args, sdr_tx_antenna, sdr_rx_antenna, tx_center_frequency, rx_center_frequency, sdr->samplerate, sdr_tx_gain, sdr_rx_gain, sdr_bandwidth);
		if (rc)
			goto error;
	}
#endif

#ifdef HAVE_SOAPY
	if (sdr_use_soapy) {
		rc = soapy_open(sdr_channel, sdr_device_args, sdr_stream_args, sdr_tune_args, sdr_tx_antenna, sdr_rx_antenna, tx_center_frequency, rx_center_frequency, sdr->samplerate, sdr_tx_gain, sdr_rx_gain, sdr_bandwidth);
		if (rc)
			goto error;
	}
#endif

	return sdr;

error:
	sdr_close(sdr);
	return NULL;
}

/* start streaming */
int sdr_start(void __attribute__((__unused__)) *inst)
{
//	sdr_t *sdr = (sdr_t *)inst;

#ifdef HAVE_UHD
	if (sdr_use_uhd)
		return uhd_start();
#endif
#ifdef HAVE_SOAPY
	if (sdr_use_soapy)
		return soapy_start();
#endif
	return -EINVAL;
}

void sdr_close(void *inst)
{
	sdr_t *sdr = (sdr_t *)inst;

#ifdef HAVE_UHD
	if (sdr_use_uhd)
		uhd_close();
#endif

#ifdef HAVE_SOAPY
	if (sdr_use_soapy)
		soapy_close();
#endif

	if (sdr) {
		wave_destroy_record(&sdr->wave_rx_rec);
		wave_destroy_record(&sdr->wave_tx_rec);
		wave_destroy_playback(&sdr->wave_rx_play);
		wave_destroy_playback(&sdr->wave_tx_play);
		free(sdr->chan);
		free(sdr);
		sdr = NULL;
	}
}

int sdr_write(void *inst, sample_t **samples, int num, enum paging_signal __attribute__((unused)) *paging_signal, int *on, int channels)
{
	sdr_t *sdr = (sdr_t *)inst;
	float buffer[num * 2], *buff = NULL;
	int c, s, ss;
	int sent = 0;

	if (channels != sdr->channels && channels != 0) {
		PDEBUG(DSDR, DEBUG_ERROR, "Invalid number of channels, please fix!\n");
		abort();
	}

	/* process all channels */
	if (channels) {
		memset(buffer, 0, sizeof(buffer));
		buff = buffer;
		for (c = 0; c < channels; c++) {
			/* switch to paging channel, if requested */
			if (on[c] && sdr->paging_channel)
				fm_modulate(&sdr->chan[sdr->paging_channel].mod, samples[c], num, buff);
			else
				fm_modulate(&sdr->chan[c].mod, samples[c], num, buff);
		}
	} else {
		buff = (float *)samples;
	}

	if (sdr->wave_tx_rec.fp) {
		sample_t spl[2][num], *spl_list[2] = { spl[0], spl[1] };
		for (s = 0, ss = 0; s < num; s++) {
			spl[0][s] = buff[ss++];
			spl[1][s] = buff[ss++];
		}
		wave_write(&sdr->wave_tx_rec, spl_list, num);
	}
	if (sdr->wave_tx_play.fp) {
		sample_t spl[2][num], *spl_list[2] = { spl[0], spl[1] };
		wave_read(&sdr->wave_tx_play, spl_list, num);
		for (s = 0, ss = 0; s < num; s++) {
			buff[ss++] = spl[0][s];
			buff[ss++] = spl[1][s];
		}
	}

#ifdef HAVE_UHD
	if (sdr_use_uhd)
		sent = uhd_send(buff, num);
#endif
#ifdef HAVE_SOAPY
	if (sdr_use_soapy)
		sent = soapy_send(buff, num);
#endif
	if (sent < 0)
		return sent;
	
	return sent;
}

int sdr_read(void *inst, sample_t **samples, int num, int channels)
{
	sdr_t *sdr = (sdr_t *)inst;
	float buffer[num * 2], *buff = NULL;
	int count = 0;
	int c, s, ss;

	if (channels) {
		buff = buffer;
	} else {
		buff = (float *)samples;
	}

#ifdef HAVE_UHD
	if (sdr_use_uhd)
		count = uhd_receive(buff, num);
#endif
#ifdef HAVE_SOAPY
	if (sdr_use_soapy)
		count = soapy_receive(buff, num);
#endif
	if (count <= 0)
		return count;

	if (sdr->wave_rx_rec.fp) {
		sample_t spl[2][count], *spl_list[2] = { spl[0], spl[1] };
		for (s = 0, ss = 0; s < count; s++) {
			spl[0][s] = buff[ss++];
			spl[1][s] = buff[ss++];
		}
		wave_write(&sdr->wave_rx_rec, spl_list, count);
	}
	if (sdr->wave_rx_play.fp) {
		sample_t spl[2][count], *spl_list[2] = { spl[0], spl[1] };
		wave_read(&sdr->wave_rx_play, spl_list, count);
		for (s = 0, ss = 0; s < count; s++) {
			buff[ss++] = spl[0][s];
			buff[ss++] = spl[1][s];
		}
	}
	display_iq(buff, count);
	display_spectrum(buff, count);

	if (channels) {
		for (c = 0; c < channels; c++)
			fm_demodulate(&sdr->chan[c].demod, samples[c], count, buff);
	}

	return count;
}

/* how many delay (in audio sample duration) do we have in the buffer */
int sdr_get_tosend(void __attribute__((__unused__)) *inst, int latspl)
{
//	sdr_t *sdr = (sdr_t *)inst;
	int count = 0;

#ifdef HAVE_UHD
	if (sdr_use_uhd)
		count = uhd_get_tosend(latspl);
#endif
#ifdef HAVE_SOAPY
	if (sdr_use_soapy)
		count = soapy_get_tosend(latspl);
#endif
	if (count < 0)
		return count;

	return count;
}


