/* A-Netz signal processing
 *
 * (C) 2016 by Andreas Eversberg <jolly@eversberg.eu>
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

#define CHAN anetz->sender.kanal

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "../common/debug.h"
#include "../common/timer.h"
#include "../common/call.h"
#include "../common/goertzel.h"
#include "anetz.h"
#include "dsp.h"

#define PI		3.1415927

/* signaling */
#define BANDWIDTH	15000.0	/* maximum bandwidth */
#define TX_PEAK_TONE	8192.0	/* peak amplitude for all tones */
#define CHUNK_DURATION	0.010	/* 10 ms */

// FIXME: how long until we detect a tone?
#define TONE_DETECT_TH	8	/* chunk intervals to detect continuous tone */

/* carrier loss detection */
#define LOSS_INTERVAL	100	/* filter steps (chunk durations) for one second interval */
#define LOSS_TIME	12	/* duration of signal loss before release */

/* two signaling tones */
static double fsk_tones[2] = {
	2280.0,
	1750.0,
};

/* table for fast sine generation */
int dsp_sine_tone[256];

/* global init for audio processing */
void dsp_init(void)
{
	int i;
	double s;

	PDEBUG(DDSP, DEBUG_DEBUG, "Generating sine tables.\n");
	for (i = 0; i < 256; i++) {
		s = sin((double)i / 256.0 * 2.0 * PI);
		dsp_sine_tone[i] = (int)(s * TX_PEAK_TONE);
	}

	if (TX_PEAK_TONE > 32767.0) {
		fprintf(stderr, "TX_PEAK_TONE definition too high, please fix!\n");
		abort();
	}
}

/* Init transceiver instance. */
int dsp_init_sender(anetz_t *anetz, double page_gain, int page_sequence)
{
	int16_t *spl;
	double coeff;
	int i;
	double tone;

	PDEBUG_CHAN(DDSP, DEBUG_DEBUG, "Init DSP for 'Sender'.\n");

	/* set deviation and modulation parameters */
	anetz->sender.bandwidth = BANDWIDTH;
	anetz->sender.sample_deviation = 11000.0 / (double)TX_PEAK_TONE;

	anetz->page_gain = page_gain;
	if (page_gain * TX_PEAK_TONE > 32767.0) {
		page_gain = 32767.0 / TX_PEAK_TONE;
		PDEBUG_CHAN(DDSP, DEBUG_NOTICE, "Highest possible gain of paging tones is %.1f dB.\n", log10(page_gain) * 20);
	}
	anetz->page_sequence = page_sequence;

	audio_init_loss(&anetz->sender.loss, LOSS_INTERVAL, anetz->sender.loss_volume, LOSS_TIME);

	anetz->samples_per_chunk = anetz->sender.samplerate * CHUNK_DURATION;
	PDEBUG(DDSP, DEBUG_DEBUG, "Using %d samples per chunk duration.\n", anetz->samples_per_chunk);
	spl = calloc(1, anetz->samples_per_chunk << 1);
	if (!spl) {
		PDEBUG(DDSP, DEBUG_ERROR, "No memory!\n");
		return -ENOMEM;
	}
	anetz->fsk_filter_spl = spl;

	anetz->tone_detected = -1;

	for (i = 0; i < 2; i++) {
		coeff = 2.0 * cos(2.0 * PI * fsk_tones[i] / (double)anetz->sender.samplerate);
		anetz->fsk_tone_coeff[i] = coeff * 32768.0;
		PDEBUG(DDSP, DEBUG_DEBUG, "RX %.0f Hz coeff = %d\n", fsk_tones[i], (int)anetz->fsk_tone_coeff[i]);
	}
	tone = fsk_tones[(anetz->sender.loopback == 0) ? 0 : 1];
	anetz->tone_phaseshift256 = 256.0 / ((double)anetz->sender.samplerate / tone);
	PDEBUG(DDSP, DEBUG_DEBUG, "TX %.0f Hz phaseshift = %.4f\n", tone, anetz->tone_phaseshift256);

	return 0;
}

/* Cleanup transceiver instance. */
void dsp_cleanup_sender(anetz_t *anetz)
{
	PDEBUG_CHAN(DDSP, DEBUG_DEBUG, "Cleanup DSP for 'Sender'.\n");

	if (anetz->fsk_filter_spl) {
		free(anetz->fsk_filter_spl);
		anetz->fsk_filter_spl = NULL;
	}
}

/* Count duration of tone and indicate detection/loss to protocol handler. */
static void fsk_receive_tone(anetz_t *anetz, int tone, int goodtone, double level)
{
	/* lost tone because it is not good anymore or has changed */
	if (!goodtone || tone != anetz->tone_detected) {
		if (anetz->tone_count >= TONE_DETECT_TH) {
			PDEBUG_CHAN(DDSP, DEBUG_INFO, "Lost %.0f Hz tone after %.0f ms.\n", fsk_tones[anetz->tone_detected], 1000.0 * CHUNK_DURATION * anetz->tone_count);
			anetz_receive_tone(anetz, -1);
		}
		if (goodtone)
			anetz->tone_detected = tone;
		else
			anetz->tone_detected = -1;
		anetz->tone_count = 0;

		return;
	}

	anetz->tone_count++;

	if (anetz->tone_count >= TONE_DETECT_TH)
		audio_reset_loss(&anetz->sender.loss);
	if (anetz->tone_count == TONE_DETECT_TH) {
		PDEBUG_CHAN(DDSP, DEBUG_INFO, "Detecting continuous %.0f Hz tone. (level = %d%%)\n", fsk_tones[anetz->tone_detected], (int)(level * 100.0 + 0.5));
		anetz_receive_tone(anetz, anetz->tone_detected);
	}
}

/* Filter one chunk of audio an detect tone, quality and loss of signal. */
static void fsk_decode_chunk(anetz_t *anetz, int16_t *spl, int max)
{
	double level, result[2];

	level = audio_level(spl, max);

	if (audio_detect_loss(&anetz->sender.loss, level))
		anetz_loss_indication(anetz);

	audio_goertzel(spl, max, 0, anetz->fsk_tone_coeff, result, 2);

	/* show quality of tone */
	if (anetz->sender.loopback) {
		/* adjust level, so we get peak of sine curve */
		PDEBUG_CHAN(DDSP, DEBUG_NOTICE, "Tone %.0f: Level=%3.0f%% Quality=%3.0f%%\n", fsk_tones[1], level / 0.63662 * 100.0 * 32768.0 / TX_PEAK_TONE, result[1] / level * 100.0);
	}
	if (level / 0.63 > 0.05 && result[0] / level > 0.5)
		PDEBUG_CHAN(DDSP, DEBUG_INFO, "Tone %.0f: Level=%3.0f%% Quality=%3.0f%%\n", fsk_tones[0], level / 0.63662 * 100.0 * 32768.0 / TX_PEAK_TONE, result[0] / level * 100.0);

	/* adjust level, so we get peak of sine curve */
	/* indicate detected tone */
	if (level / 0.63 > 0.05 && result[0] / level > 0.5)
		fsk_receive_tone(anetz, 0, 1, level / 0.63662 * 32768.0 / TX_PEAK_TONE);
	else if (level / 0.63 > 0.05 && result[1] / level > 0.5)
		fsk_receive_tone(anetz, 1, 1, level / 0.63662 * 32768.0 / TX_PEAK_TONE);
	else
		fsk_receive_tone(anetz, -1, 0, level / 0.63662 * 32768.0 / TX_PEAK_TONE);
}

/* Process received audio stream from radio unit. */
void sender_receive(sender_t *sender, int16_t *samples, int length)
{
	anetz_t *anetz = (anetz_t *) sender;
	int16_t *spl;
	int max, pos;
	int i;

	/* write received samples to decode buffer */
	max = anetz->samples_per_chunk;
	pos = anetz->fsk_filter_pos;
	spl = anetz->fsk_filter_spl;
	for (i = 0; i < length; i++) {
		spl[pos++] = samples[i];
		if (pos == max) {
			pos = 0;
			fsk_decode_chunk(anetz, spl, max);
		}
	}
	anetz->fsk_filter_pos = pos;

	/* Forward audio to network (call process). */
	if (anetz->dsp_mode == DSP_MODE_AUDIO && anetz->callref) {
		int16_t down[length]; /* more than enough */
		int count;

		count = samplerate_downsample(&anetz->sender.srstate, samples, length, down);
		spl = anetz->sender.rxbuf;
		pos = anetz->sender.rxbuf_pos;
		for (i = 0; i < count; i++) {
			spl[pos++] = down[i];
			if (pos == 160) {
				call_tx_audio(anetz->callref, spl, 160);
				pos = 0;
			}
		}
		anetz->sender.rxbuf_pos = pos;
	} else
		anetz->sender.rxbuf_pos = 0;
}

/* Set 4 paging frequencies */
void dsp_set_paging(anetz_t *anetz, double *freq)
{
	int i;

	for (i = 0; i < 4; i++) {
		anetz->paging_phaseshift256[i] = 256.0 / ((double)anetz->sender.samplerate / freq[i]);
		anetz->paging_phase256[i] = 0;
	}
}

/* Generate audio stream of 4 simultanious paging tones. Keep phase for next call of function.
 * Use TX_PEAK_TONE*page_gain for all tones, which gives peak of 1/4th for each individual tone. */
static void fsk_paging_tone(anetz_t *anetz, int16_t *samples, int length)
{
	double phaseshift[4], phase[4];
	int i;
	double sample;

	for (i = 0; i < 4; i++) {
		phaseshift[i] = anetz->paging_phaseshift256[i];
		phase[i] = anetz->paging_phase256[i];
	}

	for (i = 0; i < length; i++) {
		sample	= (int32_t)dsp_sine_tone[((uint8_t)phase[0]) & 0xff]
			+ (int32_t)dsp_sine_tone[((uint8_t)phase[1]) & 0xff]
			+ (int32_t)dsp_sine_tone[((uint8_t)phase[2]) & 0xff]
			+ (int32_t)dsp_sine_tone[((uint8_t)phase[3]) & 0xff];
		*samples++ = sample / 4.0 * anetz->page_gain;
		phase[0] += phaseshift[0];
		phase[1] += phaseshift[1];
		phase[2] += phaseshift[2];
		phase[3] += phaseshift[3];
		if (phase[0] >= 256) phase[0] -= 256;
		if (phase[1] >= 256) phase[1] -= 256;
		if (phase[2] >= 256) phase[2] -= 256;
		if (phase[3] >= 256) phase[3] -= 256;
	}

	for (i = 0; i < 4; i++) {
		anetz->paging_phase256[i] = phase[i];
	}
}

/* Generate audio stream of 4 sequenced paging tones. Keep phase for next call
 * of function.
 *
 * Use TX_PEAK_PAGE for each tone, that is four times higher per tone.
 *
 * Click removal when changing tones that have individual phase:
 * When tone changes to next tone, a transition of 2ms is performed. The last
 * tone is faded out and the new tone faded in.
 */
static void fsk_paging_tone_sequence(anetz_t *anetz, int16_t *samples, int length, int numspl)
{
	double phaseshift[4], phase[4];
	int i;
	int tone, count, transition;

	for (i = 0; i < 4; i++) {
		phaseshift[i] = anetz->paging_phaseshift256[i];
		phase[i] = anetz->paging_phase256[i];
	}
	tone = anetz->paging_tone;
	count = anetz->paging_count;
	transition = anetz->paging_transition;

	while (length) {
		/* use tone, but during transition of tones, keep phase 0 degrees (high level) until next tone reaches 0 degrees (high level) */
		if (!transition)
			*samples++ = dsp_sine_tone[((uint8_t)phase[tone]) & 0xff] * anetz->page_gain;
		else {
			/* fade between old an new tone */
			*samples++
				= (double)dsp_sine_tone[((uint8_t)phase[(tone - 1) & 3]) & 0xff] * (double)(transition - count) / (double)transition / 2.0 * anetz->page_gain
				+ (double)dsp_sine_tone[((uint8_t)phase[tone]) & 0xff] * (double)count / (double)transition / 2.0 * anetz->page_gain;
		}
		phase[0] += phaseshift[0];
		phase[1] += phaseshift[1];
		phase[2] += phaseshift[2];
		phase[3] += phaseshift[3];
		if (phase[0] >= 256) phase[0] -= 256;
		if (phase[1] >= 256) phase[1] -= 256;
		if (phase[2] >= 256) phase[2] -= 256;
		if (phase[3] >= 256) phase[3] -= 256;
		count++;
		if (transition && count == transition) {
			transition = 0;
			/* reset counter again, when transition ends */
			count = 0;
		}
		if (count >= numspl) {
			/* start transition to next tone (lasts 2 ms) */
			transition = anetz->sender.samplerate / 500;
			/* reset counter here, when transition starts */
			count = 0;
			if (++tone == 4)
				tone = 0;
		}
		length--;
	}

	for (i = 0; i < 4; i++) {
		anetz->paging_phase256[i] = phase[i];
	}
	anetz->paging_tone = tone;
	anetz->paging_count = count;
	anetz->paging_transition = transition;
}

/* Generate audio stream from tone. Keep phase for next call of function. */
static void fsk_tone(anetz_t *anetz, int16_t *samples, int length)
{
	double phaseshift, phase;
	int i;

	phaseshift = anetz->tone_phaseshift256;
	phase = anetz->tone_phase256;

	for (i = 0; i < length; i++) {
		*samples++ = dsp_sine_tone[((uint8_t)phase) & 0xff];
		phase += phaseshift;
		if (phase >= 256)
			phase -= 256;
	}

	anetz->tone_phase256 = phase;
}

/* Provide stream of audio toward radio unit */
void sender_send(sender_t *sender, int16_t *samples, int length)
{
	anetz_t *anetz = (anetz_t *) sender;

	switch (anetz->dsp_mode) {
	case DSP_MODE_SILENCE:
		memset(samples, 0, length * sizeof(*samples));
		break;
	case DSP_MODE_AUDIO:
		jitter_load(&anetz->sender.dejitter, samples, length);
		break;
	case DSP_MODE_TONE:
		fsk_tone(anetz, samples, length);
		break;
	case DSP_MODE_PAGING:
		if (anetz->page_sequence)
			fsk_paging_tone_sequence(anetz, samples, length, anetz->page_sequence * anetz->sender.samplerate / 1000);
		else
			fsk_paging_tone(anetz, samples, length);
		break;
	}
}

const char *anetz_dsp_mode_name(enum dsp_mode mode)
{
        static char invalid[16];

	switch (mode) {
	case DSP_MODE_SILENCE:
		return "SILENCE";
	case DSP_MODE_AUDIO:
		return "AUDIO";
	case DSP_MODE_TONE:
		return "TONE";
	case DSP_MODE_PAGING:
		return "PAGING";
	}

	sprintf(invalid, "invalid(%d)", mode);
	return invalid;
}

void anetz_set_dsp_mode(anetz_t *anetz, enum dsp_mode mode, int detect_reset)
{
	PDEBUG_CHAN(DDSP, DEBUG_DEBUG, "DSP mode %s -> %s\n", anetz_dsp_mode_name(anetz->dsp_mode), anetz_dsp_mode_name(mode));
	anetz->dsp_mode = mode;
	/* reset sequence paging */
	anetz->paging_tone = 0;
	anetz->paging_count = 0;
	anetz->paging_transition = 0;
	/* reset tone detector */
	if (detect_reset)
		anetz->tone_detected = -1;
}

