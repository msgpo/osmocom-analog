/* B-Netz signal processing
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

#define CHAN bnetz->sender.kanal

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "../common/sample.h"
#include "../common/debug.h"
#include "../common/call.h"
#include "bnetz.h"
#include "dsp.h"

#define PI		3.1415927

/* Notes on TX_PEAK_FSK level:
 *
 * At 2000 Hz the deviation shall be 4 kHz, so with emphasis the deviation
 * at 1000 Hz would be theoretically 2 kHz. This is factor 0.714 below
 * 2.8 kHz deviation we want at dBm0.
 */

/* signaling */
#define MAX_DEVIATION	4000.0
#define MAX_MODULATION	3000.0
#define DBM0_DEVIATION	2800.0	/* deviation of dBm0 at 1 kHz */
#define TX_PEAK_FSK	(4000.0 / 2000.0 * 1000.0 / DBM0_DEVIATION)
#define TX_PEAK_METER	(2000.0 / 2900.0 * 1000.0 / DBM0_DEVIATION) /* FIXME: what is the metering pulse deviation??? we use half of the 4kHz deviation, so we can still use -6dB of the speech level */
#define DAMPEN_METER	0.5	/* use -6dB to dampen speech while sending metering pulse (according to FTZ 1727 Pfl 32 Clause 3.2.6.6.5) */
#define MAX_DISPLAY	1.4	/* something above dBm0 */
#define BIT_RATE	100.0
#define BIT_ADJUST	0.5	/* full adjustment on bit change */
#define F0		2070.0
#define F1		1950.0
#define METERING_HZ	2900	/* metering pulse frequency */
#define TONE_DETECT_TH	7	/* 70 milliseconds to detect continuous tone */

/* carrier loss detection */
#define MUTE_TIME	0.1	/* time to mute after loosing signal */
#define LOSS_TIME	12.5	/* duration of signal loss before release (according to FTZ 1727 Pfl 32 Clause 3.2.3.2) */

/* table for fast sine generation */
static sample_t dsp_metering[65536];

/* global init for FSK */
void dsp_init(void)
{
	int i;

	PDEBUG(DDSP, DEBUG_DEBUG, "Generating sine table for metering tone.\n");
	for (i = 0; i < 65536; i++)
		dsp_metering[i] = sin((double)i / 65536.0 * 2.0 * PI) * TX_PEAK_METER;
}

static int fsk_send_bit(void *inst);
static void fsk_receive_bit(void *inst, int bit, double quality, double level);

/* Init transceiver instance. */
int dsp_init_sender(bnetz_t *bnetz, double squelch_db)
{
	PDEBUG_CHAN(DDSP, DEBUG_DEBUG, "Init DSP for 'Sender'.\n");

	/* init squelch */
	squelch_init(&bnetz->squelch, bnetz->sender.kanal, squelch_db, MUTE_TIME, LOSS_TIME);

	/* set modulation parameters */
	sender_set_fm(&bnetz->sender, MAX_DEVIATION, MAX_MODULATION, DBM0_DEVIATION, MAX_DISPLAY);

	PDEBUG(DDSP, DEBUG_DEBUG, "Using FSK level of %.3f (%.3f KHz deviation @ 2000 Hz)\n", TX_PEAK_FSK, 4.0);

	/* init fsk */
	if (fsk_init(&bnetz->fsk, bnetz, fsk_send_bit, fsk_receive_bit, bnetz->sender.samplerate, BIT_RATE, F0, F1, TX_PEAK_FSK, 0, BIT_ADJUST) < 0) {
		PDEBUG_CHAN(DDSP, DEBUG_DEBUG, "FSK init failed!\n");
		return -EINVAL;
	}

	bnetz->tone_detected = -1;

	/* metering tone */
	bnetz->meter_phaseshift65536 = 65536.0 / ((double)bnetz->sender.samplerate / METERING_HZ);
	PDEBUG(DDSP, DEBUG_DEBUG, "dial_phaseshift = %.4f\n", bnetz->meter_phaseshift65536);

	bnetz->dmp_tone_level = display_measurements_add(&bnetz->sender, "Tone Level", "%.1f %%", DISPLAY_MEAS_LAST, DISPLAY_MEAS_LEFT, 0.0, 150.0, 100.0);
	bnetz->dmp_tone_quality = display_measurements_add(&bnetz->sender, "Tone Quality", "%.1f %%", DISPLAY_MEAS_LAST, DISPLAY_MEAS_LEFT, 0.0, 100.0, 100.0);
	bnetz->dmp_frame_level = display_measurements_add(&bnetz->sender, "Frame Level", "%.1f %% (last)", DISPLAY_MEAS_LAST, DISPLAY_MEAS_LEFT, 0.0, 150.0, 100.0);
	bnetz->dmp_frame_stddev = display_measurements_add(&bnetz->sender, "Frame Stddev", "%.1f %% (last)", DISPLAY_MEAS_LAST, DISPLAY_MEAS_LEFT, 0.0, 100.0, 100.0);
	bnetz->dmp_frame_quality = display_measurements_add(&bnetz->sender, "Frame Quality", "%.1f %% (last)", DISPLAY_MEAS_LAST, DISPLAY_MEAS_LEFT, 0.0, 100.0, 100.0);

	return 0;
}

/* Cleanup transceiver instance. */
void dsp_cleanup_sender(bnetz_t *bnetz)
{
	PDEBUG_CHAN(DDSP, DEBUG_DEBUG, "Cleanup DSP for 'Sender'.\n");

	fsk_cleanup(&bnetz->fsk);
}

/* Count duration of tone and indicate detection/loss to protocol handler. */
static void fsk_receive_tone(bnetz_t *bnetz, int bit, int goodtone, double level, double quality)
{
	/* lost tone because it is not good anymore or has changed */
	if (!goodtone || bit != bnetz->tone_detected) {
		if (bnetz->tone_count >= TONE_DETECT_TH) {
			PDEBUG_CHAN(DDSP, DEBUG_DEBUG, "Lost F%d tone after %d ms.\n", bnetz->tone_detected, bnetz->tone_count);
			bnetz_receive_tone(bnetz, -1);
		}
		if (goodtone)
			bnetz->tone_detected = bit;
		else
			bnetz->tone_detected = -1;
		bnetz->tone_count = 0;

		return;
	}

	bnetz->tone_count++;

	if (bnetz->tone_count == TONE_DETECT_TH) {
		PDEBUG_CHAN(DDSP, DEBUG_INFO, "Detecting continuous tone: F%d Level=%3.0f%% Quality=%3.0f%%\n", bnetz->tone_detected, level * 100.0, quality * 100.0);
		/* must reset, so we will not get corrupt first digit */
		bnetz->rx_telegramm = bnetz->tone_detected * 0xffff;
		bnetz_receive_tone(bnetz, bnetz->tone_detected);
	}
}

/* Collect 16 data bits (digit) and check for sync mark '01110'. */
static void fsk_receive_bit(void *inst, int bit, double quality, double level)
{
	double level_avg, level_stddev, quality_avg;
	bnetz_t *bnetz = (bnetz_t *)inst;
	int i;

	/* normalize FSK level */
	level /= TX_PEAK_FSK;

	/* update measurements */
	display_measurements_update(bnetz->dmp_tone_level, level * 100.0 , 0.0);
	display_measurements_update(bnetz->dmp_tone_quality, quality * 100.0, 0.0);

	/* continuous tone detection */
	if (level > 0.10 && quality > 0.10) {
		fsk_receive_tone(bnetz, bit, 1, level, quality);
	} else
		fsk_receive_tone(bnetz, bit, 0, level, quality);

	/* collect bits */
	if (level < 0.05)
		return;
	bnetz->rx_telegramm = (bnetz->rx_telegramm << 1) | bit;
	bnetz->rx_telegramm_quality[bnetz->rx_telegramm_qualidx] = quality;
	bnetz->rx_telegramm_level[bnetz->rx_telegramm_qualidx] = level;
	if (++bnetz->rx_telegramm_qualidx == 16)
		bnetz->rx_telegramm_qualidx = 0;

	/* check if pattern 01110xxxxxxxxxxx matches */
	if ((bnetz->rx_telegramm & 0xf800) != 0x7000)
		return;

	/* average level and quality */
	level_avg = level_stddev = quality_avg = 0;
	for (i = 0; i < 16; i++) {
		level_avg += bnetz->rx_telegramm_level[bnetz->rx_telegramm_qualidx];
		quality_avg += bnetz->rx_telegramm_quality[bnetz->rx_telegramm_qualidx];
		if (++bnetz->rx_telegramm_qualidx == 16)
			bnetz->rx_telegramm_qualidx = 0;
	}
	level_avg /= 16.0; quality_avg /= 16.0;
	for (i = 0; i < 16; i++) {
		level = bnetz->rx_telegramm_level[bnetz->rx_telegramm_qualidx];
		level_stddev += (level - level_avg) * (level - level_avg);
		if (++bnetz->rx_telegramm_qualidx == 16)
			bnetz->rx_telegramm_qualidx = 0;
	}
	level_stddev = sqrt(level_stddev / 16.0);

	/* update measurements */
	display_measurements_update(bnetz->dmp_frame_level, level_avg * 100.0 , 0.0);
	display_measurements_update(bnetz->dmp_frame_stddev, level_stddev / level_avg * 100.0, 0.0);
	display_measurements_update(bnetz->dmp_frame_quality, quality_avg * 100.0, 0.0);

	/* receive telegramm */
	bnetz_receive_telegramm(bnetz, bnetz->rx_telegramm, level_avg, level_stddev, quality_avg);
}

/* Process received audio stream from radio unit. */
void sender_receive(sender_t *sender, sample_t *samples, int length, double rf_level_db)
{
	bnetz_t *bnetz = (bnetz_t *) sender;
	sample_t *spl;
	int pos;
	int i;

	/* fsk/tone signal */
	fsk_receive(&bnetz->fsk, samples, length);

	/* process signal mute/loss, without signalling tone / FSK frames */
	switch (squelch(&bnetz->squelch, rf_level_db, (double)length / (double)bnetz->sender.samplerate)) {
	case SQUELCH_LOSS:
		bnetz_loss_indication(bnetz, LOSS_TIME);
		// fall through:
	case SQUELCH_MUTE:
		memset(samples, 0, sizeof(*samples) * length);
		break;
	default:
		break;
	}

	if ((bnetz->dsp_mode == DSP_MODE_AUDIO
	  || bnetz->dsp_mode == DSP_MODE_AUDIO_METER) && bnetz->callref) {
		int count;

		count = samplerate_downsample(&bnetz->sender.srstate, samples, length);
		spl = bnetz->sender.rxbuf;
		pos = bnetz->sender.rxbuf_pos;
		for (i = 0; i < count; i++) {
			spl[pos++] = samples[i];
			if (pos == 160) {
				call_up_audio(bnetz->callref, spl, 160);
				pos = 0;
			}
		}
		bnetz->sender.rxbuf_pos = pos;
	} else
		bnetz->sender.rxbuf_pos = 0;
}

static int fsk_send_bit(void *inst)
{
	bnetz_t *bnetz = (bnetz_t *)inst;

	/* send frame bit (prio) */
	switch (bnetz->dsp_mode) {
	case DSP_MODE_TELEGRAMM:
		if (!bnetz->tx_telegramm || bnetz->tx_telegramm_pos == 16) {
			/* request frame */
			bnetz->tx_telegramm = bnetz_get_telegramm(bnetz);
			if (!bnetz->tx_telegramm) {
				PDEBUG_CHAN(DDSP, DEBUG_DEBUG, "Stop sending 'Telegramm'.\n");
				return -1;
			}
			bnetz->tx_telegramm_pos = 0;
		}

		return bnetz->tx_telegramm[bnetz->tx_telegramm_pos++];
	case DSP_MODE_0:
		return 0; /* F0 */
	case DSP_MODE_1:
		return 1; /* F1 */
	default:
		return -1; // should never happen
	}
}

/* Add metering pulse tone to audio stream. Keep phase for next call of function. */
static void metering_tone(bnetz_t *bnetz, sample_t *samples, int length)
{
        double phaseshift, phase;
	int i;

	phaseshift = bnetz->meter_phaseshift65536;
	phase = bnetz->meter_phase65536;

	for (i = 0; i < length; i++) {
		/* Add metering pulse, also dampen audio level by 6 dB */
		*samples = (*samples) * DAMPEN_METER + dsp_metering[(uint16_t)phase];
		samples++;
		phase += phaseshift;
		if (phase >= 65536)
			phase -= 65536;
	}

	bnetz->meter_phase65536 = phase;
}

/* Provide stream of audio toward radio unit */
void sender_send(sender_t *sender, sample_t *samples, uint8_t *power, int length)
{
	bnetz_t *bnetz = (bnetz_t *) sender;
	int count;

	memset(power, 1, length);

again:
	switch (bnetz->dsp_mode) {
	case DSP_MODE_SILENCE:
		memset(samples, 0, length * sizeof(*samples));
		break;
	case DSP_MODE_AUDIO:
	case DSP_MODE_AUDIO_METER:
		jitter_load(&bnetz->sender.dejitter, samples, length);
		if (bnetz->dsp_mode == DSP_MODE_AUDIO_METER)
			metering_tone(bnetz, samples, length);
		break;
	case DSP_MODE_0:
	case DSP_MODE_1:
	case DSP_MODE_TELEGRAMM:
		/* Encode tone/frame into audio stream. If frames have
		 * stopped, process again for rest of stream. */
		count = fsk_send(&bnetz->fsk, samples, length, 0);
		samples += count;
		length -= count;
		if (length)
			goto again;
		break;
	}
}

const char *bnetz_dsp_mode_name(enum dsp_mode mode)
{
        static char invalid[16];

	switch (mode) {
	case DSP_MODE_SILENCE:
		return "SILENCE";
	case DSP_MODE_AUDIO:
		return "AUDIO";
	case DSP_MODE_AUDIO_METER:
		return "AUDIO + METERING PULSE";
	case DSP_MODE_0:
		return "TONE 0";
	case DSP_MODE_1:
		return "TONE 1";
	case DSP_MODE_TELEGRAMM:
		return "TELEGRAMM";
	}

	sprintf(invalid, "invalid(%d)", mode);
	return invalid;
}

void bnetz_set_dsp_mode(bnetz_t *bnetz, enum dsp_mode mode)
{
	/* reset telegramm */
	if (mode == DSP_MODE_TELEGRAMM && bnetz->dsp_mode != mode) {
		bnetz->tx_telegramm = 0;
		fsk_tx_reset(&bnetz->fsk);
	}
	
	PDEBUG_CHAN(DDSP, DEBUG_DEBUG, "DSP mode %s -> %s\n", bnetz_dsp_mode_name(bnetz->dsp_mode), bnetz_dsp_mode_name(mode));
	bnetz->dsp_mode = mode;
}

