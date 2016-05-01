
enum amps_ie {
	AMPS_IE_010111,
	AMPS_IE_1,
	AMPS_IE_11,
	AMPS_IE_1111,
	AMPS_IE_ACT,
	AMPS_IE_AUTH,
	AMPS_IE_AUTHBS,
	AMPS_IE_AUTHR,
	AMPS_IE_AUTHU,
	AMPS_IE_Acked_Data,
	AMPS_IE_Async_Data,
	AMPS_IE_BIS,
	AMPS_IE_BSCAP,
	AMPS_IE_BSPC,
	AMPS_IE_CHAN,
	AMPS_IE_CHANPOS,
	AMPS_IE_CHARACTER_1,
	AMPS_IE_CHARACTER_2,
	AMPS_IE_CHARACTER_3,
	AMPS_IE_CMAC,
	AMPS_IE_CMAX_1,
	AMPS_IE_COUNT,
	AMPS_IE_CPA,
	AMPS_IE_CPN_RL,
	AMPS_IE_CRC,
	AMPS_IE_CRI_E11,
	AMPS_IE_CRI_E12,
	AMPS_IE_CRI_E13,
	AMPS_IE_CRI_E14,
	AMPS_IE_CRI_E21,
	AMPS_IE_CRI_E22,
	AMPS_IE_CRI_E23,
	AMPS_IE_CRI_E24,
	AMPS_IE_CRI_E31,
	AMPS_IE_CRI_E32,
	AMPS_IE_CRI_E33,
	AMPS_IE_CRI_E34,
	AMPS_IE_CRI_E41,
	AMPS_IE_CRI_E42,
	AMPS_IE_CRI_E43,
	AMPS_IE_CRI_E44,
	AMPS_IE_CRI_E51,
	AMPS_IE_CRI_E52,
	AMPS_IE_CRI_E53,
	AMPS_IE_CRI_E54,
	AMPS_IE_CRI_E61,
	AMPS_IE_CRI_E62,
	AMPS_IE_CRI_E63,
	AMPS_IE_CRI_E64,
	AMPS_IE_CRI_E71,
	AMPS_IE_CRI_E72,
	AMPS_IE_CRI_E73,
	AMPS_IE_CRI_E74,
	AMPS_IE_CRI_E81,
	AMPS_IE_CRI_E82,
	AMPS_IE_CRI_E83,
	AMPS_IE_CRI_E84,
	AMPS_IE_DCC,
	AMPS_IE_DIGIT_1,
	AMPS_IE_DIGIT_10,
	AMPS_IE_DIGIT_11,
	AMPS_IE_DIGIT_12,
	AMPS_IE_DIGIT_13,
	AMPS_IE_DIGIT_14,
	AMPS_IE_DIGIT_15,
	AMPS_IE_DIGIT_16,
	AMPS_IE_DIGIT_17,
	AMPS_IE_DIGIT_18,
	AMPS_IE_DIGIT_19,
	AMPS_IE_DIGIT_2,
	AMPS_IE_DIGIT_20,
	AMPS_IE_DIGIT_21,
	AMPS_IE_DIGIT_22,
	AMPS_IE_DIGIT_23,
	AMPS_IE_DIGIT_24,
	AMPS_IE_DIGIT_25,
	AMPS_IE_DIGIT_26,
	AMPS_IE_DIGIT_27,
	AMPS_IE_DIGIT_28,
	AMPS_IE_DIGIT_29,
	AMPS_IE_DIGIT_3,
	AMPS_IE_DIGIT_30,
	AMPS_IE_DIGIT_31,
	AMPS_IE_DIGIT_32,
	AMPS_IE_DIGIT_4,
	AMPS_IE_DIGIT_5,
	AMPS_IE_DIGIT_6,
	AMPS_IE_DIGIT_7,
	AMPS_IE_DIGIT_8,
	AMPS_IE_DIGIT_9,
	AMPS_IE_DMAC,
	AMPS_IE_DTX,
	AMPS_IE_DTX_Support,
	AMPS_IE_DVCC,
	AMPS_IE_Data_Part,
	AMPS_IE_Data_Privacy,
	AMPS_IE_E,
	AMPS_IE_EC,
	AMPS_IE_EF,
	AMPS_IE_END,
	AMPS_IE_EP,
	AMPS_IE_ER,
	AMPS_IE_ESN,
	AMPS_IE_F,
	AMPS_IE_G3_Fax,
	AMPS_IE_HDVCC,
	AMPS_IE_Hyperband,
	AMPS_IE_LOCAID,
	AMPS_IE_LOCAL_CONTROL,
	AMPS_IE_LOCAL_MSG_TYPE,
	AMPS_IE_LREG,
	AMPS_IE_LT,
	AMPS_IE_MAXBUSY_OTHER,
	AMPS_IE_MAXBUSY_PGR,
	AMPS_IE_MAXSZTR_OTHER,
	AMPS_IE_MAXSZTR_PGR,
	AMPS_IE_MEM,
	AMPS_IE_MIN1,
	AMPS_IE_MIN2,
	AMPS_IE_MPCI,
	AMPS_IE_MSCAP,
	AMPS_IE_MSPC,
	AMPS_IE_N_1,
	AMPS_IE_NAWC	,
	AMPS_IE_NEWACC,
	AMPS_IE_NULL,
	AMPS_IE_OHD,
	AMPS_IE_OLC_0,
	AMPS_IE_OLC_1,
	AMPS_IE_OLC_10,
	AMPS_IE_OLC_11,
	AMPS_IE_OLC_12,
	AMPS_IE_OLC_13,
	AMPS_IE_OLC_14,
	AMPS_IE_OLC_15,
	AMPS_IE_OLC_2,
	AMPS_IE_OLC_3,
	AMPS_IE_OLC_4,
	AMPS_IE_OLC_5,
	AMPS_IE_OLC_6,
	AMPS_IE_OLC_7,
	AMPS_IE_OLC_8,
	AMPS_IE_OLC_9,
	AMPS_IE_ORDER,
	AMPS_IE_ORDQ,
	AMPS_IE_P,
	AMPS_IE_PCI,
	AMPS_IE_PCI_HOME,
	AMPS_IE_PCI_ROAM,
	AMPS_IE_PDREG,
	AMPS_IE_PI,
	AMPS_IE_PM,
	AMPS_IE_PM_D,
	AMPS_IE_PSCC,
	AMPS_IE_PUREG,
	AMPS_IE_PVI,
	AMPS_IE_RAND1_A,
	AMPS_IE_RAND1_B,
	AMPS_IE_RANDBS,
	AMPS_IE_RANDC,
	AMPS_IE_RANDSSD_1,
	AMPS_IE_RANDSSD_2,
	AMPS_IE_RANDSSD_3,
	AMPS_IE_RANDU,
	AMPS_IE_RCF,
	AMPS_IE_REGH,
	AMPS_IE_REGID,
	AMPS_IE_REGINCR,
	AMPS_IE_REGR,
	AMPS_IE_RLP,
	AMPS_IE_RL_W,
	AMPS_IE_RSVD,
	AMPS_IE_S,
	AMPS_IE_SAP,
	AMPS_IE_SBI,
	AMPS_IE_SCC,
	AMPS_IE_SCM,
	AMPS_IE_SDCC1,
	AMPS_IE_SDCC2,
	AMPS_IE_SI,
	AMPS_IE_SID1,
	AMPS_IE_SIGNAL,
	AMPS_IE_Service_Code,
	AMPS_IE_T,
	AMPS_IE_T1T2,
	AMPS_IE_TA,
	AMPS_IE_TCI1,
	AMPS_IE_TCI21,
	AMPS_IE_TCI22,
	AMPS_IE_TCI23,
	AMPS_IE_TCI24,
	AMPS_IE_TCI31,
	AMPS_IE_TCI32,
	AMPS_IE_TCI33,
	AMPS_IE_TCI34,
	AMPS_IE_TCI41,
	AMPS_IE_TCI42,
	AMPS_IE_TCI43,
	AMPS_IE_TCI44,
	AMPS_IE_TCI5,
	AMPS_IE_VMAC,
	AMPS_IE_WFOM,
	AMPS_IE_NUM
};

typedef struct amps_frame {
	enum amps_ie	ie[AMPS_IE_NUM];
} frame_t;

void init_frame(void);
uint64_t amps_encode_word1_system(uint8_t dcc, uint16_t sid1, uint8_t ep, uint8_t auth, uint8_t pci, uint8_t nawc);
uint64_t amps_encode_word2_system(uint8_t dcc, uint8_t s, uint8_t e, uint8_t regh, uint8_t regr, uint8_t dtx, uint8_t n_1, uint8_t rcf, uint8_t cpa, uint8_t cmax_1, uint8_t end);
uint64_t amps_encode_registration_id(uint8_t dcc, uint32_t regid, uint8_t end);
uint64_t amps_encode_registration_increment(uint8_t dcc, uint16_t regincr, uint8_t end);
uint64_t amps_encode_location_area(uint8_t dcc, uint8_t pureg, uint8_t pdreg, uint8_t lreg, uint16_t locaid, uint8_t end);
uint64_t amps_encode_new_access_channel_set(uint8_t dcc, uint16_t newacc, uint8_t end);
uint64_t amps_encode_overload_control(uint8_t dcc, uint8_t *olc, uint8_t end);
uint64_t amps_encode_access_type(uint8_t dcc, uint8_t bis, uint8_t pci_home, uint8_t pci_roam, uint8_t bspc, uint8_t bscap, uint8_t end);
uint64_t amps_encode_access_attempt(uint8_t dcc, uint8_t maxbusy_pgr, uint8_t maxsztr_pgr, uint8_t maxbusy_other, uint8_t maxsztr_other, uint8_t end);
const char *amps_encode_frame_focc(amps_t *amps);
const char *amps_encode_frame_fvc(amps_t *amps);
int amps_decode_frame(amps_t *amps, const char *bits, int count, double level, double quality, int negative);

