
int update_db(cnetz_t *cnetz, uint8_t futln_nat, uint8_t futln_fuvst, uint16_t futln_rest, int *futelg_bit, int *extended, int busy, int failed);
int find_db(uint8_t futln_nat, uint8_t futln_fuvst, uint16_t futln_rest, int *futelg_bit, int *extended);
void flush_db(void);
void dump_db(void);

