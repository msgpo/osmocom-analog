/* Common part for main.c of each base station type
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sched.h>
#include <unistd.h>
#include <math.h>
#include <termios.h>
#include <errno.h>
#include "../libsample/sample.h"
#include "main_mobile.h"
#include "../libdebug/debug.h"
#include "sender.h"
#include "../libtimer/timer.h"
#include "call.h"
#include "../libmncc/mncc_console.h"
#include "../libmncc/mncc_sock.h"
#include "../libmncc/mncc_cross.h"
#ifdef HAVE_SDR
#include "../libsdr/sdr.h"
#include "../libsdr/sdr_config.h"
#endif
#include "../liboptions/options.h"
#include "../libfm/fm.h"
#include "image.h"

#define DEFAULT_LO_OFFSET -1000000.0

static int got_init = 0;

/* common mobile settings */
int num_kanal = 0;
const char *kanal[MAX_SENDER];
int num_audiodev = 0;
const char *audiodev[MAX_SENDER] = { "hw:0,0" };
int use_sdr = 0;
static const char *call_audiodev = "";
int samplerate = 48000;
static int call_samplerate = 48000;
int interval = 1;
int latency = 50;
int uses_emphasis = 1;
int do_pre_emphasis = 0;
int do_de_emphasis = 0;
double rx_gain = 1.0;
static int echo_test = 0;
static int use_mncc_sock = 0;
static int use_mncc_cross = 0;
const char *mncc_name = "";
int send_patterns = 1;
static int release_on_disconnect = 1;
int loopback = 0;
int rt_prio = 1;
int fast_math = 0;
const char *write_tx_wave = NULL;
const char *write_rx_wave = NULL;
const char *read_tx_wave = NULL;
const char *read_rx_wave = NULL;
const char *console_digits = "0123456789";

void main_mobile_init(void)
{
	got_init = 1;
#ifdef HAVE_SDR
	sdr_config_init(DEFAULT_LO_OFFSET);
#endif
}

void main_mobile_print_help(const char *arg0, const char *ext_usage)
{
	printf("Usage: %s -k <kanal/channel> %s[options] [station-id]\n", arg0, ext_usage);
	printf("\nGlobal options:\n");
	/*      -                                                                             - */
	printf(" -h --help\n");
	printf("        This help\n");
	printf(" -v --verbose <level> | <level>,<category>[,<category>[,...]] | list\n");
	printf("        Use 'list' to get a list of all levels and categories\n");
	printf("        Verbose level: digit of debug level (default = '%d')\n", debuglevel);
	printf("        Verbose level+category: level digit followed by one or more categories\n");
	printf("        -> If no category is specified, all categories are selected\n");
	printf(" -k --kanal <channel>\n");
	printf(" -k --channel <channel>\n");
	printf("        Channel (German = Kanal) number of \"Sender\" (German = Transceiver)\n");
	printf(" -a --audio-device hw:<card>,<device>\n");
	printf("        Sound card and device number (default = '%s')\n", audiodev[0]);
	printf("        Don't set it for SDR!\n");
	printf(" -s --samplerate <rate>\n");
	printf("        Sample rate of sound device (default = '%d')\n", samplerate);
	printf(" -i --interval 1..25\n");
	printf("        Interval of processing loop in ms (default = '%d' ms)\n", interval);
	printf("        Use 25 to drastically reduce CPU usage. In case of buffer underrun,\n");
	printf("        increase latency accordingly.\n");
	printf(" -b --buffer <ms>\n");
	printf("        How many milliseconds are processed in advance (default = '%d')\n", latency);
    if (uses_emphasis) {
	printf(" -p --pre-emphasis\n");
	printf("        Enable pre-emphasis, if you directly connect to the oscillator of the\n");
	printf("        transmitter. (No pre-emphasis done by the transmitter.)\n");
	printf(" -d --de-emphasis\n");
	printf("        Enable de-emphasis, if you directly connect to the discriminator of\n");
	printf("        the receiver. (No de-emphasis done by the receiver.)\n");
    }
	printf(" -g --rx-gain <dB>\n");
	printf("        Raise receiver RX level by given gain in dB. This is useful if input\n");
	printf("        level of the sound device is too low, even after setting maximum level\n");
	printf("        with the mixer settings. (Works with sound card only.)\n");
	printf(" -e --echo-test\n");
	printf("        Use echo test, to send back audio from mobile phone's microphone to\n");
	printf("        the speaker. (German: 'Blasprobe').\n");
	printf(" -c --call-device hw:<card>,<device>\n");
	printf("        Sound card and device number for headset (default = '%s')\n", call_audiodev);
	printf("    --call-samplerate <rate>\n");
	printf("        Sample rate of sound device for headset (default = '%d')\n", call_samplerate);
	printf(" -x --mncc-cross\n");
	printf("        Enable built-in call forwarding between mobiles. Be sure to have\n");
	printf("        at least one control channel and two voice channels. Alternatively\n");
	printf("        use one combined control+voice channel and one voice channels.\n");
	printf(" -m --mncc-sock\n");
	printf("        Disable built-in call contol and offer socket (to LCR)\n");
	printf("    --mncc-name <name>\n");
	printf("        '/tmp/bsc_mncc' is used by default, give name to change socket to\n");
	printf("        '/tmp/bsc_mncc_<name>'. (Useful to run multiple networks.)\n");
	printf(" -t --tones 0 | 1\n");
	printf("        Connect call on setup/release to provide classic tones towards fixed\n");
	printf("        network (default = '%d')\n", send_patterns);
	printf(" -l --loopback <type>\n");
	printf("        Loopback test: 1 = internal | 2 = external | 3 = echo\n");
	printf(" -r --realtime <prio>\n");
	printf("        Set prio: 0 to diable, 99 for maximum (default = %d)\n", rt_prio);
	printf("    --fast-math\n");
	printf("        Use fast math approximation for slow CPU / ARM based systems.\n");
	printf("    --write-rx-wave <file>\n");
	printf("        Write received audio to given wave file.\n");
	printf("    --write-tx-wave <file>\n");
	printf("        Write transmitted audio to given wave file.\n");
	printf("    --read-rx-wave <file>\n");
	printf("        Replace received audio by given wave file.\n");
	printf("    --read-tx-wave <file>\n");
	printf("        Replace transmitted audio by given wave file.\n");
#ifdef HAVE_SDR
	printf("    --limesdr\n");
	printf("        Auto-select several required options for LimeSDR\n");
	printf("    --limesdr-mini\n");
	printf("        Auto-select several required options for LimeSDR Mini\n");
	sdr_config_print_help();
#endif
	printf("\nNetwork specific options:\n");
}

void main_mobile_print_hotkeys(void)
{
	printf("\n");
	printf("Press digits '0'..'9' and then 'd' key to dial towards mobile station.\n");
	printf("Press 'h' key to hangup.\n");
	printf("Press 'w' key to toggle display of RX wave form.\n");
	printf("Press 'c' key to toggle display of channel status.\n");
	printf("Press 'm' key to toggle display of measurement value.\n");
#ifdef HAVE_SDR
	sdr_config_print_hotkeys();
#endif
}

#define	OPT_CHANNEL		1000
#define	OPT_WRITE_RX_WAVE	1001
#define	OPT_WRITE_TX_WAVE	1002
#define	OPT_READ_RX_WAVE	1003
#define	OPT_READ_TX_WAVE	1004
#define	OPT_CALL_SAMPLERATE	1005
#define	OPT_MNCC_NAME		1006
#define	OPT_FAST_MATH		1007
#define	OPT_LIMESDR		1100
#define	OPT_LIMESDR_MINI	1101

void main_mobile_add_options(void)
{
	option_add('h', "help", 0);
	option_add('v', "debug", 1);
	option_add('k', "kanal", 1);
	option_add(OPT_CHANNEL, "channel", 1);
	option_add('a', "audio-device", 1);
	option_add('s', "samplerate", 1);
	option_add('i', "interval", 1);
	option_add('b', "buffer", 1);
	option_add('p', "pre-emphasis", 0);
	option_add('d', "de-emphasis", 0);
	option_add('g', "rx-gain", 1);
	option_add('e', "echo-test", 0);
	option_add('x', "mncc-cross", 0);
	option_add('m', "mncc-sock", 0);
	option_add(OPT_MNCC_NAME, "mncc-name", 1);
	option_add('c', "call-device", 1);
	option_add(OPT_CALL_SAMPLERATE, "call-samplerate", 1);
	option_add('t', "tones", 1);
	option_add('l', "loopback", 1);
	option_add('r', "realtime", 1);
	option_add(OPT_FAST_MATH, "fast-math", 0);
	option_add(OPT_WRITE_RX_WAVE, "write-rx-wave", 1);
	option_add(OPT_WRITE_TX_WAVE, "write-tx-wave", 1);
	option_add(OPT_READ_RX_WAVE, "read-rx-wave", 1);
	option_add(OPT_READ_TX_WAVE, "read-tx-wave", 1);
#ifdef HAVE_SDR
	option_add(OPT_LIMESDR, "limesdr", 0);
	option_add(OPT_LIMESDR_MINI, "limesdr-mini", 0);
	sdr_config_add_options();
#endif
};

void print_help(const char *arg0);

int main_mobile_handle_options(int short_option, int argi, char **argv)
{
	double gain_db;
	int rc;

	switch (short_option) {
	case 'h':
		print_help(argv[0]);
		return 0;
	case 'v':
		if (!strcasecmp(argv[argi], "list")) {
	                debug_list_cat();
			return 0;
		}
		rc = parse_debug_opt(argv[argi]);
		if (rc < 0) {
			fprintf(stderr, "Failed to parse debug option, please use -h for help.\n");
			return rc;
		}
		break;
	case 'k':
	case OPT_CHANNEL:
		OPT_ARRAY(num_kanal, kanal, argv[argi])
		break;
	case 'a':
		OPT_ARRAY(num_audiodev, audiodev, strdup(argv[argi]))
		break;
	case 's':
		samplerate = atoi(argv[argi]);
		break;
	case 'i':
		interval = atoi(argv[argi]);
		if (interval < 1)
			interval = 1;
		if (interval > 25)
			interval = 25;
		break;
	case 'b':
		latency = atoi(argv[argi]);
		break;
	case 'p':
		if (!uses_emphasis) {
			no_emph:
			fprintf(stderr, "This network does not use emphasis, please do not enable pre- or de-emphasis! Disable emphasis on transceiver, if possible.\n");
			return -EINVAL;
		}
		do_pre_emphasis = 1;
		break;
	case 'd':
		if (!uses_emphasis)
			goto no_emph;
		do_de_emphasis = 1;
		break;
	case 'g':
		gain_db = atof(argv[argi]);
		if (gain_db < 0.0) {
			fprintf(stderr, "Given gain is below 0. To reduce RX signal, use sound card's mixer (or resistor net)!\n");
			return -EINVAL;
		}
		rx_gain = pow(10, gain_db / 20.0);
		break;
	case 'e':
		echo_test = 1;
		break;
	case 'x':
		use_mncc_cross = 1;
		break;
	case 'm':
		use_mncc_sock = 1;
		break;
	case OPT_MNCC_NAME:
		mncc_name = strdup(argv[argi]);
		break;
	case 'c':
		call_audiodev = strdup(argv[argi]);
		break;
	case OPT_CALL_SAMPLERATE:
		call_samplerate = atoi(argv[argi]);
		break;
	case 't':
		send_patterns = atoi(argv[argi]);
		break;
	case 'l':
		loopback = atoi(argv[argi]);
		break;
	case 'r':
		rt_prio = atoi(argv[argi]);
		break;
	case OPT_FAST_MATH:
		fast_math = 1;
		break;
	case OPT_WRITE_RX_WAVE:
		write_rx_wave = strdup(argv[argi]);
		break;
	case OPT_WRITE_TX_WAVE:
		write_tx_wave = strdup(argv[argi]);
		break;
	case OPT_READ_RX_WAVE:
		read_rx_wave = strdup(argv[argi]);
		break;
	case OPT_READ_TX_WAVE:
		read_tx_wave = strdup(argv[argi]);
		break;
#ifdef HAVE_SDR
	case OPT_LIMESDR:
		{
			char *argv_lime[] = { argv[0],
				"--sdr-soapy",
				"--sdr-rx-antenna", "LNAL",
				"--sdr-rx-gain", "30",
				"--sdr-tx-gain", "30",
				"--sdr-samplerate", "5000000",
				"--sdr-bandwidth", "15000000",
				"-s", "200000",
			};
			int argc_lime = sizeof(argv_lime) / sizeof (*argv_lime);
			return options_command_line(argc_lime, argv_lime, main_mobile_handle_options);
		}
	case OPT_LIMESDR_MINI:
		{
			char *argv_lime[] = { argv[0],
				"--sdr-soapy",
				"--sdr-rx-antenna", "LNAW",
				"--sdr-tx-antenna", "BAND2",
				"--sdr-rx-gain", "25",
				"--sdr-tx-gain", "30",
				"--sdr-samplerate", "5000000",
				"--sdr-bandwidth", "15000000",
				"-s", "200000",
			};
			int argc_lime = sizeof(argv_lime) / sizeof (*argv_lime);
			return options_command_line(argc_lime, argv_lime, main_mobile_handle_options);
		}
#endif
	default:
#ifdef HAVE_SDR
		return sdr_config_handle_options(short_option, argi, argv);
#else
		return -EINVAL;
#endif
	}

	return 1;
}

/* global variable to quit main loop */
int quit = 0;

void sighandler(int sigset)
{
	if (sigset == SIGHUP)
		return;
	if (sigset == SIGPIPE)
		return;

	if (clear_console_text)
		clear_console_text();
	printf("Signal received: %d\n", sigset);

	quit = 1;
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

/* Loop through all transceiver instances of one network. */
void main_mobile(int *quit, int latency, int interval, void (*myhandler)(void), const char *station_id, int station_id_digits)
{
	int latspl;
	sender_t *sender;
	double last_time_call = 0, begin_time, now, sleep;
	struct termios term, term_orig;
	int c;
	int rc;

	if (!got_init) {
		fprintf(stderr, "main_mobile_init was not called, please fix!\n");
		abort();
	}

	/* latency of send buffer in samples */
	latspl = samplerate * latency / 1000;

	/* check MNCC support */
	if (use_mncc_cross && num_kanal == 1) {
		fprintf(stderr, "You selected built-in call forwarding, but only channel is used. Does this makes sense?\n");
		return;
	}
	if (use_mncc_sock && use_mncc_cross) {
		fprintf(stderr, "You selected MNCC socket interface and built-in call forwarding, but only one can be selected.\n");
		return;
	}
	if (echo_test && call_audiodev[0]) {
		fprintf(stderr, "You selected call device (headset) and echo test, but only one can be selected.\n");
		return;
	}
	if (use_mncc_sock && call_audiodev[0]) {
		fprintf(stderr, "You selected MNCC socket interface, but it cannot be used with call device (headset).\n");
		return;
	}
	if (use_mncc_cross && call_audiodev[0]) {
		fprintf(stderr, "You selected built-in call forwarding, but it cannot be used with call device (headset).\n");
		return;
	}
	if (use_mncc_sock && echo_test) {
		fprintf(stderr, "You selected MNCC socket interface, but it cannot be used with echo test.\n");
		return;
	}
	if (use_mncc_cross && echo_test) {
		fprintf(stderr, "You selected built-in call forwarding, but it cannot be used with echo test.\n");
		return;
	}

	/* init mncc */
	if (use_mncc_sock) {
		char mncc_sock_name[64];
		if (mncc_name[0]) {
			snprintf(mncc_sock_name, sizeof(mncc_sock_name), "/tmp/bsc_mncc_%s", mncc_name);
			mncc_sock_name[sizeof(mncc_sock_name) - 1] = '\0';
		} else
			strcpy(mncc_sock_name, "/tmp/bsc_mncc");
		rc = mncc_sock_init(mncc_sock_name);
		if (rc < 0) {
			fprintf(stderr, "Failed to setup MNCC socket. Quitting!\n");
			return;
		}
	} else if (use_mncc_cross) {
		rc = mncc_cross_init();
		if (rc < 0) {
			fprintf(stderr, "Failed to setup MNCC crossing process. Quitting!\n");
			return;
		}
	} else {
		console_init(station_id, call_audiodev, call_samplerate, latency, station_id_digits, loopback, echo_test, console_digits);
	}

	/* init call control instance */
	rc = call_init((use_mncc_sock) ? send_patterns : 0, release_on_disconnect);
	if (rc < 0) {
		fprintf(stderr, "Failed to create call control instance. Quitting!\n");
		return;
	}

#ifdef HAVE_SDR
	rc = sdr_configure(samplerate);
	if (rc < 0)
		return;
#endif

	/* open audio */
	if (sender_open_audio(latspl))
		return;
	if (console_open_audio(latspl))
		return;

	if (!loopback)
		print_image();

	/* real time priority */
	if (rt_prio > 0) {
		struct sched_param schedp;
		int rc;

		memset(&schedp, 0, sizeof(schedp));
		schedp.sched_priority = rt_prio;
		rc = sched_setscheduler(0, SCHED_RR, &schedp);
		if (rc)
			fprintf(stderr, "Error setting SCHED_RR with prio %d\n", rt_prio);
	}

	/* prepare terminal */
	tcgetattr(0, &term_orig);
	term = term_orig;
	term.c_lflag &= ~(ISIG|ICANON|ECHO);
	term.c_cc[VMIN]=1;
	term.c_cc[VTIME]=2;
	tcsetattr(0, TCSANOW, &term);

	/* catch signals */
	signal(SIGINT, sighandler);
	signal(SIGHUP, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGPIPE, sighandler);

	/* start streaming */
	if (sender_start_audio())
		*quit = 1;
	if (console_start_audio())
		*quit = 1;

	while(!(*quit)) {
		begin_time = get_time();

		/* process sound of all transceivers */
		for (sender = sender_head; sender; sender = sender->next) {
			/* do not process audio for an audio slave, since it is done by audio master */
			if (sender->master) /* if master is set, we are an audio slave */
				continue;
			process_sender_audio(sender, quit, latspl);
		}

		/* process timers */
		process_timer();

		/* process audio for mncc call instances */
		now = get_time();
		if (now - last_time_call >= 0.1)
			last_time_call = now;
		if (now - last_time_call >= 0.020) {
			last_time_call += 0.020;
			/* call clock every 20ms */
			call_clock();
		}

next_char:
		c = get_char();
		switch (c) {
		case 3:
			/* quit */
			if (clear_console_text)
				clear_console_text();
			printf("CTRL+c received, quitting!\n");
			*quit = 1;
			goto next_char;
		case 'w':
			/* toggle wave display */
			display_status_on(0);
			display_measurements_on(0);
#ifdef HAVE_SDR
			display_iq_on(0);
			display_spectrum_on(0);
#endif
			display_wave_on(-1);
			goto next_char;
		case 'c':
			/* toggle call state display */
			display_wave_on(0);
			display_measurements_on(0);
#ifdef HAVE_SDR
			display_iq_on(0);
			display_spectrum_on(0);
#endif
			display_status_on(-1);
			goto next_char;
		case 'm':
			/* toggle measurements display */
			display_wave_on(0);
			display_status_on(0);
#ifdef HAVE_SDR
			display_iq_on(0);
			display_spectrum_on(0);
#endif
			display_measurements_on(-1);
			goto next_char;
#ifdef HAVE_SDR
		case 'q':
			/* toggle IQ display */
			display_wave_on(0);
			display_status_on(0);
			display_measurements_on(0);
			display_spectrum_on(0);
			display_iq_on(-1);
			goto next_char;
		case 's':
			/* toggle spectrum display */
			display_wave_on(0);
			display_status_on(0);
			display_measurements_on(0);
			display_iq_on(0);
			display_spectrum_on(-1);
			goto next_char;
#endif
		case 'i':
			/* dump info */
			dump_info();
			goto next_char;
#ifdef HAVE_SDR
		case 'b':
			calibrate_bias();
			goto next_char;
#endif
		}

		/* process call control */
		if (use_mncc_sock)
			mncc_sock_handle();
		else if (use_mncc_cross)
			mncc_cross_handle();
		else
			process_console(c);

		if (myhandler)
			myhandler();

		display_measurements((double)interval / 1000.0);

		now = get_time();

		/* sleep interval */
		sleep = ((double)interval / 1000.0) - (now - begin_time);
		if (sleep > 0)
			usleep(sleep * 1000000.0);

//		now = get_time();
//		printf("duration =%.6f\n", now - begin_time);
	}

	/* reset signals */
	signal(SIGINT, SIG_DFL);
	signal(SIGHUP, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGPIPE, SIG_DFL);

	/* get rid of last entry */
	if (clear_console_text)
		clear_console_text();

	/* reset terminal */
	tcsetattr(0, TCSANOW, &term_orig);
	
	/* reset real time prio */
	if (rt_prio > 0) {
		struct sched_param schedp;

		memset(&schedp, 0, sizeof(schedp));
		schedp.sched_priority = 0;
		sched_setscheduler(0, SCHED_OTHER, &schedp);
	}

	/* cleanup call control */
	if (!use_mncc_sock && !use_mncc_cross)
		console_cleanup();

	/* close mncc socket */
	if (use_mncc_sock)
		mncc_sock_exit();

	/* close mncc forwarding */
	if (use_mncc_cross)
		mncc_cross_exit();
}

