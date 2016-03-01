/* built-in call control
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <termios.h>
#include "../common/debug.h"
#include "../common/sender.h"
#include "cause.h"
#include "call.h"
#include "mncc_sock.h"
#include "freiton.h"
#include "besetztton.h"

extern int use_mncc_sock;
extern int send_patterns;

/* stream patterns/announcements */
int16_t *ansage_27_spl = NULL;
int16_t *freiton_spl = NULL;
int16_t *besetztton_spl = NULL;
int ansage_27_size = 0;
int freiton_size = 0;
int besetztton_size = 0;

enum call_state {
	CALL_IDLE = 0,
	CALL_SETUP_MO,
	CALL_SETUP_MT,
	CALL_ALERTING,
	CALL_CONNECT,
	CALL_DISCONNECTED,
};

enum audio_pattern {
	PATTERN_NONE = 0,
	PATTERN_RINGBACK,
	PATTERN_BUSY,
	PATTERN_OUTOFORDER,
};

static int new_callref = 0; /* toward mobile */

/* built in call instance */
typedef struct call {
	int callref;
	enum call_state state;
	int disc_cause; /* cause that has been sent by transceiver instance for release */
	char station_id[6];
	char dialing[16];
	void *sound; /* headphone interface */
	int latspl; /* sample latency at sound interface */
	samplerate_t srstate; /* patterns/announcement upsampling */
	jitter_t audio; /* headphone audio dejittering */
	int audio_pos; /* position when playing patterns */
	int loopback; /* loopback test for echo */
} call_t;

static call_t call;

static void call_new_state(enum call_state state)
{
	call.state = state;
	call.audio_pos = 0;
}

static void get_call_patterns(int16_t *samples, int length, enum audio_pattern pattern)
{
	const int16_t *spl = NULL;
	int size = 0, max = 0, pos;

	switch (pattern) {
	case PATTERN_RINGBACK:
		spl = freiton_spl;
		size = freiton_size;
		max = 8 * 5000;
		break;
	case PATTERN_BUSY:
busy:
		spl = besetztton_spl;
		size = besetztton_size;
		max = 8 * 750;
		break;
	case PATTERN_OUTOFORDER:
		spl = ansage_27_spl;
		size = ansage_27_size;
		if (!spl || !size)
			goto busy;
		max = size;
		break;
	default:
		return;
	}

	/* stream sample */
	pos = call.audio_pos;
	while(length--) {
		if (pos >= size)
			*samples++ = 0;
		else
			*samples++ = spl[pos] >> 1;
		if (++pos == max)
			pos = 0;
	}
	call.audio_pos = pos;
}

static enum audio_pattern cause2pattern(int cause)
{
	int pattern;

	switch (cause) {
	case CAUSE_OUTOFORDER:
		pattern = PATTERN_OUTOFORDER;
		break;
	default:
		pattern = PATTERN_BUSY;
	}

	return pattern;
}

/* MNCC call instance */
typedef struct process {
	struct process *next;
	int callref;
	enum call_state state;
	int audio_disconnected; /* if not associated with transceiver anymore */
	enum audio_pattern pattern;
	int audio_pos;
} process_t;

static process_t *process_head = NULL;

static void create_process(int callref, int state)
{
	process_t *process;

	process = calloc(sizeof(*process), 1);
	if (!process) {
		PDEBUG(DCALL, DEBUG_ERROR, "No memory!\n");
		abort();
	}
	process->next = process_head;
	process_head = process;

	process->callref = callref;
	process->state = state;
}

static void destroy_process(int callref)
{
	process_t *process = process_head;
	process_t **process_p = &process_head;

	while (process) {
		if (process->callref == callref) {
			*process_p = process->next;
			free(process);
			return;
		}
		process_p = &process->next;
		process = process->next;
	}
	PDEBUG(DCALL, DEBUG_ERROR, "Process with callref 0x%x not found!\n", callref);
}

static int is_process(int callref)
{
	process_t *process = process_head;

	while (process) {
		if (process->callref == callref)
			return 1;
		process = process->next;
	}
	return 0;
}

static enum call_state is_process_state(int callref)
{
	process_t *process = process_head;

	while (process) {
		if (process->callref == callref)
			return process->state;
		process = process->next;
	}
	return CALL_IDLE;
}

static void set_state_process(int callref, enum call_state state)
{
	process_t *process = process_head;

	while (process) {
		if (process->callref == callref) {
			process->state = state;
			return;
		}
		process = process->next;
	}
	PDEBUG(DCALL, DEBUG_ERROR, "Process with callref 0x%x not found!\n", callref);
}

static void set_pattern_process(int callref, enum audio_pattern pattern)
{
	process_t *process = process_head;

	while (process) {
		if (process->callref == callref) {
			process->pattern = pattern;
			process->audio_pos = 0;
			return;
		}
		process = process->next;
	}
	PDEBUG(DCALL, DEBUG_ERROR, "Process with callref 0x%x not found!\n", callref);
}

/* disconnect audio, now send audio directly from pattern/announcement, not from transceiver */
static void disconnect_process(int callref, int cause)
{
	process_t *process = process_head;

	while (process) {
		if (process->callref == callref) {
			process->pattern = cause2pattern(cause);
			process->audio_disconnected = 1;
			process->audio_pos = 0;
			return;
		}
		process = process->next;
	}
	PDEBUG(DCALL, DEBUG_ERROR, "Process with callref 0x%x not found!\n", callref);
}

/* check if audio is disconnected */
static int is_process_disconnected(int callref)
{
	process_t *process = process_head;

	while (process) {
		if (process->callref == callref) {
			return process->audio_disconnected;
		}
		process = process->next;
	}
	PDEBUG(DCALL, DEBUG_DEBUG, "Process with callref 0x%x not found, this is ok!\n", callref);

	return 0;
}

/* check if pattern is set, so we send patterns and announcements */
static int is_process_pattern(int callref)
{
	process_t *process = process_head;

	while (process) {
		if (process->callref == callref) {
			return (process->pattern != PATTERN_NONE);
		}
		process = process->next;
	}
	PDEBUG(DCALL, DEBUG_DEBUG, "Process with callref 0x%x not found, this is ok!\n", callref);

	return 0;
}

static void get_process_patterns(process_t *process, int16_t *samples, int length)
{
	const int16_t *spl = NULL;
	int size = 0, max = 0, pos;

	switch (process->pattern) {
	case PATTERN_RINGBACK:
		spl = freiton_spl;
		size = freiton_size;
		max = 8 * 5000;
		break;
	case PATTERN_BUSY:
		spl = besetztton_spl;
		size = besetztton_size;
		max = 8 * 750;
		break;
	case PATTERN_OUTOFORDER:
		spl = ansage_27_spl;
		size = ansage_27_size;
		max = size;
		break;
	default:
		return;
	}

	/* stream sample */
	pos = process->audio_pos;
	while(length--) {
		if (pos >= size)
			*samples++ = 0;
		else
			*samples++ = spl[pos] >> 1;
		if (++pos == max)
			pos = 0;
	}
	process->audio_pos = pos;
}

static struct termios term_orig;

int call_init(const char *station_id, const char *sounddev, int samplerate, int latency, int loopback)
{
	struct termios term;
	int rc = 0;

	/* init common tones */
	init_freiton();
	init_besetzton();

	if (use_mncc_sock)
		return 0;

	if (!loopback) {
		tcgetattr(0, &term_orig);
		term = term_orig;
		term.c_lflag &= ~(ISIG|ICANON|ECHO);
		term.c_cc[VMIN]=1;
		term.c_cc[VTIME]=2;
		tcsetattr(0, TCSANOW, &term);
	}

	memset(&call, 0, sizeof(call));
	strncpy(call.station_id, station_id, sizeof(call.station_id) - 1);
	call.latspl = latency * samplerate / 1000;
	call.loopback = loopback;

	if (!sounddev[0])
		return 0;

	/* open sound device for call control */
	call.sound = sound_open(sounddev, samplerate);
	if (!call.sound) {
		PDEBUG(DSENDER, DEBUG_ERROR, "No sound device!\n");

		rc = -EIO;
		goto error;
	}

	rc = init_samplerate(&call.srstate, samplerate);
	if (rc < 0) {
		PDEBUG(DSENDER, DEBUG_ERROR, "Failed to init sample rate conversion!\n");
		goto error;
	}

	rc = jitter_create(&call.audio, samplerate / 5);
	if (rc < 0) {
		PDEBUG(DSENDER, DEBUG_ERROR, "Failed to create and init audio buffer!\n");
		goto error;
	}

	return 0;

error:
	call_cleanup();
	return rc;
}

void call_cleanup(void)
{
	if (use_mncc_sock)
		return;
	if (!call.loopback)
		tcsetattr(0, TCSANOW, &term_orig);

	/* close sound devoice */
	if (call.sound)
		sound_close(call.sound);

	jitter_destroy(&call.audio);

	if (process_head) {
		PDEBUG(DMNCC, DEBUG_ERROR, "Not all MNCC instances have been released!\n");
	}
}

static int get_char()
{
	struct timeval tv = {0, 0};
	fd_set fds;
	char c = 0;
	int __attribute__((__unused__)) rc;

	FD_ZERO(&fds);
	FD_SET(0, &fds);
	select(0+1, &fds, NULL, NULL, &tv);
	if (FD_ISSET(0, &fds)) {
		rc = read(0, &c, 1);
		return c;
	} else
		return -1;
}

static int process_ui(void)
{
	int c;

	c = get_char();

	/* break */
	if (c == 3)
		return 1;

	switch (call.state) {
	case CALL_IDLE:
		if (c > 0) {
			if (c >= '0' && c <= '9' && strlen(call.station_id) < 5) {
				call.station_id[strlen(call.station_id) + 1] = '\0';
				call.station_id[strlen(call.station_id)] = c;
			}
			if ((c == 8 || c == 127) && strlen(call.station_id))
				call.station_id[strlen(call.station_id) - 1] = '\0';
			if (c == 'd' && strlen(call.station_id) == 5) {
				int rc;
				int callref = ++new_callref;

				PDEBUG(DCALL, DEBUG_INFO, "Outgoing call to %s\n", call.station_id);
				call.dialing[0] = '\0';
				call_new_state(CALL_SETUP_MT);
				call.callref = callref;
				rc = call_out_setup(callref, call.station_id);
				if (rc < 0) {
					PDEBUG(DCALL, DEBUG_NOTICE, "Call rejected, cause %d\n", -rc);
					call_new_state(CALL_DISCONNECTED);
					call.callref = 0;
					call.disc_cause = -rc;
				}
			}
		}
		printf("on-hook: %s%s (enter 0..9 or d=dial)\r", call.station_id, "....." + strlen(call.station_id));
		break;
	case CALL_SETUP_MO:
	case CALL_SETUP_MT:
	case CALL_ALERTING:
	case CALL_CONNECT:
	case CALL_DISCONNECTED:
		if (c > 0) {
			if (c == 'h') {
				PDEBUG(DCALL, DEBUG_INFO, "Call hangup\n");
				call_new_state(CALL_IDLE);
				if (call.callref) {
					call_out_release(call.callref, CAUSE_NORMAL);
					call.callref = 0;
				}
			}
		}
		if (call.state == CALL_SETUP_MT)
			printf("call setup: %s (enter h=hangup)\r", call.station_id);
		if (call.state == CALL_ALERTING)
			printf("call ringing: %s (enter h=hangup)\r", call.station_id);
		if (call.state == CALL_CONNECT) {
			if (call.dialing[0])
				printf("call active: %s->%s (enter h=hangup)\r", call.station_id, call.dialing);
			else
				printf("call active: %s (enter h=hangup)\r", call.station_id);
		}
		if (call.state == CALL_DISCONNECTED)
			printf("call disconnected: %s (enter h=hangup)\r", cause_name(call.disc_cause));
		break;
	}
	fflush(stdout);

	return 0;
}

/* get keys from keyboad to control call via console
 * returns 1 on exit (ctrl+c) */
int process_call(void)
{
	if (use_mncc_sock) {
		mncc_handle();
		return 0;
	}

	if (!call.loopback) {
		if (process_ui())
			return 1;
	}

	if (!call.sound)
		return 0;
	/* handle audio, if sound device is used */

	int16_t samples[call.latspl];
	int count;
	int rc;

	count = sound_get_inbuffer(call.sound);
	if (count < 0) {
		PDEBUG(DSENDER, DEBUG_ERROR, "Failed to get samples in buffer (rc = %d)!\n", count);
		if (count == -EPIPE)
			PDEBUG(DSENDER, DEBUG_ERROR, "Trying to recover.\n");
		return 0;
	}
	if (count < call.latspl) {
		int16_t up[count];
		count = call.latspl - count;
		switch(call.state) {
		case CALL_ALERTING:
			count = count / call.srstate.factor;
			get_call_patterns(samples, count, PATTERN_RINGBACK);
			count = samplerate_upsample(&call.srstate, samples, count, up);
			/* prevent click after hangup */
			jitter_clear(&call.audio);
			break;
		case CALL_DISCONNECTED:
			count = count / call.srstate.factor;
			get_call_patterns(samples, count, cause2pattern(call.disc_cause));
			count = samplerate_upsample(&call.srstate, samples, count, up);
			/* prevent click after hangup */
			jitter_clear(&call.audio);
			break;
		default:
			jitter_load(&call.audio, up, count);
		}
		rc = sound_write(call.sound, up, up, count);
		if (rc < 0) {
			PDEBUG(DSENDER, DEBUG_ERROR, "Failed to write TX data to sound device (rc = %d)\n", rc);
			if (rc == -EPIPE)
				PDEBUG(DSENDER, DEBUG_ERROR, "Trying to recover.\n");
			return 0;
		}
	}
	count = sound_read(call.sound, samples, call.latspl);
	if (count < 0) {
		PDEBUG(DSENDER, DEBUG_ERROR, "Failed to read from sound device (rc = %d)!\n", count);
		if (count == -EPIPE)
			PDEBUG(DSENDER, DEBUG_ERROR, "Trying to recover.\n");
		return 0;
	}
	if (count) {
		int16_t down[count]; /* more than enough */

		if (call.loopback == 3)
			jitter_save(&call.audio, samples, count);
		count = samplerate_downsample(&call.srstate, samples, count, down);
		call_rx_audio(call.callref, down, count);
	}

	return 0;
}

/* Setup is received from transceiver. */
int call_in_setup(int callref, const char *callerid, const char *dialing)
{
	if (callref < 0x4000000) {
		PDEBUG(DCALL, DEBUG_ERROR, "Invalid callref from mobile station, please fix!\n");
		abort();
	}

	if (!strcmp(dialing, "0"))
		dialing = "operator";

	PDEBUG(DCALL, DEBUG_INFO, "Incomming call from '%s' to '%s'\n", callerid, dialing);

	if (use_mncc_sock) {
		uint8_t buf[sizeof(struct gsm_mncc)];
		struct gsm_mncc *mncc = (struct gsm_mncc *)buf;
		int rc;

		memset(buf, 0, sizeof(buf));
		mncc->msg_type = MNCC_SETUP_IND;
		mncc->callref = callref;
		mncc->fields |= MNCC_F_CALLING;
		strncpy(mncc->calling.number, callerid, sizeof(mncc->calling.number) - 1);
		mncc->calling.type = 4; /* caller ID is of type 'subscriber' */
		mncc->fields |= MNCC_F_CALLED;
		strncpy(mncc->called.number, dialing, sizeof(mncc->called.number) - 1);
		mncc->called.type = 0; /* dialing is of type 'unknown' */
		mncc->lchan_type = GSM_LCHAN_TCH_F;
		mncc->fields |= MNCC_F_BEARER_CAP;
		mncc->bearer_cap.speech_ver[0] = BCAP_ANALOG_8000HZ;
		mncc->bearer_cap.speech_ver[1] = -1;

		PDEBUG(DMNCC, DEBUG_INFO, "Sending MNCC call towards Network\n");

		create_process(callref, CALL_SETUP_MO);

		rc = mncc_write(buf, sizeof(struct gsm_mncc));
		if (rc < 0) {
			PDEBUG(DCALL, DEBUG_NOTICE, "We have no MNCC connection, rejecting.\n");
			destroy_process(callref);
			return -CAUSE_TEMPFAIL;
		}
		return 0;
	}

	/* setup is also allowed on disconnected call */
	if (call.state == CALL_DISCONNECTED)
		call_new_state(CALL_IDLE);
	if (call.state != CALL_IDLE) {
		PDEBUG(DCALL, DEBUG_NOTICE, "We are busy, rejecting.\n");
		return -CAUSE_BUSY;
	}
	call.callref = callref;
	call_new_state(CALL_CONNECT);
	if (callerid[0]) {
		strncpy(call.station_id, callerid, 5);
		call.station_id[5] = '\0';
	}
	strncpy(call.dialing, dialing, sizeof(call.dialing) - 1);
	call.dialing[sizeof(call.dialing) - 1] = '\0';

	return 0;
}

/* Transceiver indicates alerting. */
void call_in_alerting(int callref)
{
	PDEBUG(DCALL, DEBUG_INFO, "Call is alerting\n");

	if (use_mncc_sock) {
		uint8_t buf[sizeof(struct gsm_mncc)];
		struct gsm_mncc *mncc = (struct gsm_mncc *)buf;

		if (!send_patterns) {
			memset(buf, 0, sizeof(buf));
			mncc->msg_type = MNCC_ALERT_IND;
			mncc->callref = callref;
			PDEBUG(DMNCC, DEBUG_INFO, "Indicate MNCC alerting towards Network\n");
			mncc_write(buf, sizeof(struct gsm_mncc));
		} else
			set_pattern_process(callref, PATTERN_RINGBACK);
		return;
	}

	if (call.callref != callref) {
		PDEBUG(DCALL, DEBUG_ERROR, "invalid call ref.\n");
		call_out_release(callref, CAUSE_INVALCALLREF);
		return;
	}
	call_new_state(CALL_ALERTING);
}

/* Transceiver indicates answer. */
static void _indicate_answer(int callref, const char *connectid)
{
	uint8_t buf[sizeof(struct gsm_mncc)];
	struct gsm_mncc *mncc = (struct gsm_mncc *)buf;

	memset(buf, 0, sizeof(buf));
	mncc->msg_type = MNCC_SETUP_CNF;
	mncc->callref = callref;
	mncc->fields |= MNCC_F_CONNECTED;
	strncpy(mncc->connected.number, connectid, sizeof(mncc->connected.number) - 1);
	mncc->connected.type = 0;

	PDEBUG(DMNCC, DEBUG_INFO, "Indicate MNCC answer towards Network\n");
	mncc_write(buf, sizeof(struct gsm_mncc));
}
void call_in_answer(int callref, const char *connectid)
{
	PDEBUG(DCALL, DEBUG_INFO, "Call has been answered by '%s'\n", connectid);

	if (use_mncc_sock) {
		_indicate_answer(callref, connectid);
		set_pattern_process(callref, PATTERN_NONE);
		set_state_process(callref, CALL_CONNECT);
		return;
	}

	if (call.callref != callref) {
		PDEBUG(DCALL, DEBUG_ERROR, "invalid call ref.\n");
		call_out_release(callref, CAUSE_INVALCALLREF);
		return;
	}
	call_new_state(CALL_CONNECT);
	strncpy(call.station_id, connectid, 5);
	call.station_id[5] = '\0';
}

/* Transceiver indicates release. */
void call_in_release(int callref, int cause)
{
	PDEBUG(DCALL, DEBUG_INFO, "Call has been released with cause=%d\n", cause);

	if (use_mncc_sock) {
		uint8_t buf[sizeof(struct gsm_mncc)];
		struct gsm_mncc *mncc = (struct gsm_mncc *)buf;

		memset(buf, 0, sizeof(buf));
		mncc->msg_type = MNCC_REL_IND;
		mncc->callref = callref;
		mncc->fields |= MNCC_F_CAUSE;
		mncc->cause.location = 1; /* private local */
		mncc->cause.value = cause;

		if (is_process(callref)) {
			if (!send_patterns
			 || is_process_state(callref) == CALL_DISCONNECTED
			 || is_process_state(callref) == CALL_SETUP_MO) {
				PDEBUG(DMNCC, DEBUG_INFO, "Releasing MNCC call towards Network\n");
				destroy_process(callref);
				mncc_write(buf, sizeof(struct gsm_mncc));
			} else {
				disconnect_process(callref, cause);
			}
		} else {
			PDEBUG(DMNCC, DEBUG_INFO, "Releasing MNCC call towards Network\n");
			mncc_write(buf, sizeof(struct gsm_mncc));
		}
		return;
	}

	if (call.callref != callref) {
		PDEBUG(DCALL, DEBUG_ERROR, "invalid call ref.\n");
		/* don't send release, because caller already released */
		return;
	}
	call_new_state(CALL_DISCONNECTED);
	call.callref = 0;
	call.disc_cause = cause;
}

/* forward audio to MNCC or call instance */
void call_tx_audio(int callref, int16_t *samples, int count)
{

	if (use_mncc_sock) {
		uint8_t buf[sizeof(struct gsm_data_frame) + count * sizeof(int16_t)];
		struct gsm_data_frame *data = (struct gsm_data_frame *)buf;

		/* if we are disconnected, ignore audio */
		if (is_process_pattern(callref))
			return;

		/* forward audio */
		data->msg_type = ANALOG_8000HZ;
		data->callref = callref;
		memcpy(data->data, samples, count * sizeof(int16_t));

		mncc_write(buf, sizeof(buf));
		return;
	}

	/* save audio from transceiver to jitter buffer */
	if (call.sound) {
		int16_t up[count * call.srstate.factor];
		count = samplerate_upsample(&call.srstate, samples, count, up);
		jitter_save(&call.audio, up, count);
	}
}

/* clock that is used to transmit patterns */
void call_mncc_clock(void)
{
	process_t *process = process_head;
	uint8_t buf[sizeof(struct gsm_data_frame) + 160 * sizeof(int16_t)];
	struct gsm_data_frame *data = (struct gsm_data_frame *)buf;

	while(process) {
		if (process->pattern != PATTERN_NONE) {
			data->msg_type = ANALOG_8000HZ;
			data->callref = process->callref;
			/* try to get patterns, else copy the samples we got */
			get_process_patterns(process, (int16_t *)data->data, 160);
			mncc_write(buf, sizeof(buf));
		}
		process = process->next;
	}
}

/* mncc messages received from network */
void call_mncc_recv(uint8_t *buf, int length)
{
	struct gsm_mncc *mncc = (struct gsm_mncc *)buf;
	char number[sizeof(mncc->called.number)];
	int callref;
	int rc;

	if (mncc->msg_type == ANALOG_8000HZ) {
		struct gsm_data_frame *data = (struct gsm_data_frame *)buf;
		int count = (length - sizeof(struct gsm_data_frame)) / 2;
		/* if we are disconnected, ignore audio */
		if (is_process_pattern(data->callref))
			return;
		call_rx_audio(data->callref, (int16_t *)data->data, count);
		return;
	}

	callref = mncc->callref;
	strcpy(number, mncc->called.number);

	if (is_process_disconnected(callref)) {
		switch(mncc->msg_type) {
		case MNCC_DISC_REQ:
			PDEBUG(DMNCC, DEBUG_INFO, "Received MNCC disconnect from Network with cause %d\n", mncc->cause.value);

			PDEBUG(DCALL, DEBUG_INFO, "Call disconnected, releasing!\n");

			destroy_process(callref);

			PDEBUG(DMNCC, DEBUG_INFO, "Releasing MNCC call towards Network\n");
			mncc->msg_type = MNCC_REL_IND;
			mncc_write(buf, sizeof(struct gsm_mncc));
		break;
		case MNCC_REL_REQ:
			PDEBUG(DMNCC, DEBUG_INFO, "Received MNCC release from Network with cause %d\n", mncc->cause.value);

			PDEBUG(DCALL, DEBUG_INFO, "Call released\n");

			destroy_process(callref);
			break;
		}
		return;
	}

	switch(mncc->msg_type) {
	case MNCC_SETUP_REQ:
		PDEBUG(DMNCC, DEBUG_INFO, "Received MNCC call from Network to '%s'\n", mncc->called.number);

		if (mncc->callref >= 0x4000000) {
			fprintf(stderr, "Invalid callref from network, please fix!\n");
			abort();
		}

		PDEBUG(DMNCC, DEBUG_INFO, "Confirming MNCC call to Network\n");
		memset(buf, 0, length);
		mncc->msg_type = MNCC_CALL_CONF_IND;
		mncc->callref = callref;
		mncc->lchan_type = GSM_LCHAN_TCH_F;
		mncc->fields |= MNCC_F_BEARER_CAP;
		mncc->bearer_cap.speech_ver[0] = BCAP_ANALOG_8000HZ;
		mncc->bearer_cap.speech_ver[1] = -1;

		mncc_write(buf, sizeof(struct gsm_mncc));

		PDEBUG(DCALL, DEBUG_INFO, "Outgoing call from to '%s'\n", number);

		create_process(callref, CALL_SETUP_MT);

		rc = call_out_setup(callref, number);
		if (rc < 0) {
			PDEBUG(DCALL, DEBUG_NOTICE, "Call rejected, cause %d\n", -rc);
			if (send_patterns) {
				PDEBUG(DCALL, DEBUG_DEBUG, "Early connecting after setup\n");
				_indicate_answer(callref, number);
				disconnect_process(callref, -rc);
				break;
			}
			PDEBUG(DMNCC, DEBUG_INFO, "Rejecting MNCC call towards Network (cause=%d)\n", -rc);
			memset(buf, 0, length);
			mncc->msg_type = MNCC_REL_IND;
			mncc->callref = callref;
			mncc->fields |= MNCC_F_CAUSE;
			mncc->cause.location = 1; /* private local */
			mncc->cause.value = -rc;
			mncc_write(buf, sizeof(struct gsm_mncc));
			destroy_process(callref);
			break;
		}

		if (send_patterns) {
			PDEBUG(DCALL, DEBUG_DEBUG, "Early connecting after setup\n");
			set_state_process(callref, CALL_CONNECT);
			_indicate_answer(callref, number);
			break;
		}
		break;
	case MNCC_SETUP_RSP:
		PDEBUG(DMNCC, DEBUG_INFO, "Received MNCC answer from Network\n");
		set_state_process(callref, CALL_CONNECT);
		break;
	case MNCC_DISC_REQ:
		PDEBUG(DMNCC, DEBUG_INFO, "Received MNCC disconnect from Network with cause %d\n", mncc->cause.value);

		set_state_process(callref, CALL_DISCONNECTED);
		PDEBUG(DCALL, DEBUG_INFO, "Call disconnected\n");
		call_out_disconnect(callref, mncc->cause.value);
		break;
	case MNCC_REL_REQ:
		PDEBUG(DMNCC, DEBUG_INFO, "Received MNCC release from Network with cause %d\n", mncc->cause.value);

		destroy_process(callref);
		PDEBUG(DCALL, DEBUG_INFO, "Call released\n");
		call_out_release(callref, mncc->cause.value);
		break;
	}
}

/* break down of MNCC socket */
void call_mncc_flush(void)
{
	while(process_head) {
		PDEBUG(DMNCC, DEBUG_NOTICE, "MNCC socket closed, releasing call\n");
		call_out_release(process_head->callref, CAUSE_TEMPFAIL);
		destroy_process(process_head->callref);
		/* note: callref is released by sender's instance */
	}
}

