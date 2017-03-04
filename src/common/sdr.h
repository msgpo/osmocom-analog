
int sdr_init(int sdr_uhd, int sdr_soapy, const char *device_args, double rx_gain, double tx_gain, const char *write_iq_rx_wave, const char *write_iq_tx_wave, const char *read_iq_rx_wave);
int sdr_start(void *inst);
void *sdr_open(const char *audiodev, double *tx_frequency, double *rx_frequency, int channels, double paging_frequency, int samplerate, double bandwidth, double sample_deviation);
void sdr_close(void *inst);
int sdr_write(void *inst, sample_t **samples, int num, enum paging_signal *paging_signal, int *on, int channels);
int sdr_read(void *inst, sample_t **samples, int num, int channels);
int sdr_get_tosend(void *inst, int latspl);

