
void *sound_open(const char *device, int samplerate);
void sound_close(void *inst);
int sound_write(void *inst, int16_t *samples_left, int16_t *samples_right, int num);
int sound_read(void *inst, int16_t *samples, int num);
int sound_get_inbuffer(void *inst);
