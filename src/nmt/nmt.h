#include "../common/sender.h"
#include "../common/compandor.h"
#include "../common/dtmf.h"

enum dsp_mode {
	DSP_MODE_DIALTONE,	/* stream dial tone to mobile phone */
	DSP_MODE_AUDIO,		/* stream audio */
	DSP_MODE_SILENCE,	/* stream nothing */
	DSP_MODE_FRAME,		/* send frames */
	DSP_MODE_DTMF,		/* send DTMF tones */
};

enum nmt_chan_type {
	CHAN_TYPE_CC,		/* calling channel */
	CHAN_TYPE_TC,		/* traffic channel */
	CHAN_TYPE_CC_TC,	/* combined CC + TC */
	CHAN_TYPE_TEST,		/* test channel */
};

enum nmt_state {
	STATE_NULL,		/* power off state */
	STATE_IDLE,		/* channel is not in use */
	STATE_ROAMING_IDENT,	/* seizure received, waiting for identity */
	STATE_ROAMING_CONFIRM,	/* identity received, sending confirm */
	STATE_MO_IDENT,		/* seizure of mobile originated call, waiting for identity */
	STATE_MO_CONFIRM,	/* identity received, sending confirm */
	STATE_MO_DIALING,	/* receiving digits from phone */
	STATE_MO_COMPLETE,	/* all digits received, completing call */
	STATE_MT_PAGING,	/* paging mobile phone */
	STATE_MT_CHANNEL,	/* assigning traffic channel */
	STATE_MT_IDENT,		/* waiting for identity */
	STATE_MT_RINGING,	/* mobile phone is ringing, waiting for answer */
	STATE_MT_COMPLETE,	/* mobile phone has answered, completing call */
	STATE_ACTIVE,		/* during active call */
	STATE_MO_RELEASE,	/* acknowlegde release from mobile phone */
	STATE_MT_RELEASE,	/* sending release toward mobile phone */
};

enum nmt_active_state {
	ACTIVE_STATE_VOICE,	/* normal conversation */
	ACTIVE_STATE_MFT_IN,	/* ack MFT converter in */
	ACTIVE_STATE_MFT,	/* receive digits in MFT mode */
	ACTIVE_STATE_MFT_OUT,	/* ack MFT converter out */
};

enum nmt_direction {
	MTX_TO_MS,
	MTX_TO_BS,
	MTX_TO_XX,
	BS_TO_MTX,
	MS_TO_MTX,
	XX_TO_MTX,
};

typedef struct nmt_sysinfo {
	enum nmt_chan_type	chan_type;		/* channel type */
	int			ms_power;		/* ms power level 3 = full */
	uint8_t			traffic_area;		/* two digits traffic area, encoded as YY */
	uint8_t			area_no;		/* Area no. 1..4, 0 = no Area no. */
} nmt_sysinfo_t;

typedef struct nmt_subscriber {
	/* NOTE: country must be followed by number, so both represent a string */
	char			country;		/* country digit */
	char			number[7];		/* phone suffix */
	char			password[4];		/* phone's password + '\0' */
	int			coinbox;		/* phone is a coinbox and accept tariff information */
} nmt_subscriber_t;

const char *nmt_dir_name(enum nmt_direction dir);

typedef struct nmt {
	sender_t		sender;
	nmt_sysinfo_t		sysinfo;
	compandor_t		cstate;
	dtmf_t			dtmf;

	/* sender's states */
	enum nmt_state		state;
	enum nmt_active_state	active_state;
	nmt_subscriber_t	subscriber;		/* current subscriber */
	struct timer		timer;
	int			rx_frame_count;		/* receive frame counter */
	int			tx_frame_count;		/* transmit frame counter */
	char			dialing[33];		/* dialed digits */
	int			page_try;		/* number of paging try */

	/* special state for paging on different CC */
	struct nmt		*page_for_nmt;		/* only page and assign channel for nmt instance as set */
	int			mt_channel;		/* channel to use */

	/* features */
	int			compandor;		/* if compandor shall be used */
	int			supervisory;		/* if set, use supervisory signal 1..4 */

	/* dsp states */
	enum dsp_mode		dsp_mode;		/* current mode: audio, durable tone 0 or 1, paging */
	int			samples_per_bit;	/* number of samples for one bit (1200 Baud) */
	int			super_samples;		/* number of samples in buffer for supervisory detection */
	int			fsk_coeff[2];		/* coefficient k = 2*cos(2*PI*f/samplerate), k << 15 */
	int			super_coeff[5];		/* coefficient for supervisory signal */
	int16_t			*fsk_sine[2][2];	/* 4 pointers to 4 precalc. sine curves */
	int			fsk_polarity;		/* current polarity state of bit */
	int			samples_per_chunk;	/* how many samples lasts one chunk */
	int16_t			*fsk_filter_spl;	/* array with samples_per_chunk */
	int			fsk_filter_pos;		/* current sample position in filter_spl */
	int			fsk_filter_step;	/* number of samples for each analyzation */
	int			fsk_filter_bit;		/* last bit, so we detect a bit change */
	int			fsk_filter_sample;	/* count until it is time to sample bit */
	uint16_t		fsk_filter_sync;	/* shift register to detect sync */
	int			fsk_filter_in_sync;	/* if we are in sync and receive bits */
	int			fsk_filter_mute;	/* mute count down after sync */
	char			fsk_filter_frame[141];	/* receive frame (one extra byte to terminate string) */
	int			fsk_filter_count;	/* next bit to receive */
	double			fsk_filter_level[256];	/* level infos */
	double			fsk_filter_quality[256];/* quality infos */
	int16_t			*super_filter_spl;	/* array with sample buffer for supervisory detection */
	int			super_filter_pos;	/* current sample position in filter_spl */
	double			super_phaseshift256[4];	/* how much the phase of sine wave changes per sample */
	double			super_phase256;		/* current phase */
	double			dial_phaseshift256;	/* how much the phase of sine wave changes per sample */
	double			dial_phase256;		/* current phase */
	int			frame;			/* set, if there is a valid frame */
	int16_t			*frame_spl;		/* 166 * fsk_bit_length */
	int			frame_pos;		/* current sample position in frame_spl */
	uint64_t		rx_sample_count;	/* sample counter */
	uint64_t		rx_sample_count_current;/* sample counter of current frame */
	uint64_t		rx_sample_count_last;	/* sample counter of last frame */
	int			super_detected;		/* current detection state flag */
	int			super_detect_count;	/* current number of consecutive detections/losses */
	int			mft_num;		/* counter for digit */
} nmt_t;

void nmt_channel_list(void);
int nmt_channel_by_short_name(const char *short_name);
const char *chan_type_short_name(enum nmt_chan_type chan_type);
const char *chan_type_long_name(enum nmt_chan_type chan_type);
double nmt_channel2freq(int channel, int uplink);
void nmt_country_list(void);
uint8_t nmt_country_by_short_name(const char *short_name);
int nmt_create(int channel, enum nmt_chan_type chan_type, const char *sounddev, int samplerate, int cross_channels, double rx_gain, int pre_emphasis, int de_emphasis, const char *write_wave, const char *read_wave, uint8_t ms_power, uint8_t traffic_area, uint8_t area_no, int compandor, int supervisory, int loopback);
void nmt_destroy(sender_t *sender);
void nmt_receive_frame(nmt_t *nmt, const char *bits, double quality, double level, double frames_elapsed);
const char *nmt_get_frame(nmt_t *nmt);
void nmt_rx_super(nmt_t *nmt, int tone, double quality);

