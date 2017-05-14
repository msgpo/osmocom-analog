
extern int num_kanal;
extern int kanal[];
extern int num_audiodev;
extern const char *audiodev[];
extern int use_sdr;
extern const char *call_audiodev;
extern int samplerate;
extern int interval;
extern int latency;
extern int uses_emphasis;
extern int do_pre_emphasis;
extern int do_de_emphasis;
extern double rx_gain;
extern int use_mncc_sock;
extern int send_patterns;
extern int loopback;
extern int rt_prio;
extern const char *write_rx_wave;
extern const char *write_tx_wave;
extern const char *read_rx_wave;

void print_help(const char *arg0);
void print_help_common(const char *arg0, const char *ext_usage);
void print_hotkeys_common(void);
extern struct option *long_options;
extern char *optstring;
void set_options_common(const char *optstring_special, struct option *long_options_special);
void opt_switch_common(int c, char *arg0, int *skip_args);

#define OPT_ARRAY(num_name, name, value) \
{ \
	if (num_name == MAX_SENDER) { \
		fprintf(stderr, "Too many channels defined!\n"); \
		exit(0); \
	} \
	name[num_name++] = value; \
}

extern int quit;
void sighandler(int sigset);

void main_common(int *quit, int latency, int interval, void (*myhandler)(void), const char *station_id, int station_id_digits);

void dump_info(void);

