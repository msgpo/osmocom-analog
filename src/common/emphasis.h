typedef struct emphasis {
	struct {
		double x_last;
		double factor;
		double amp;
	} p;
	struct {
		double y_last;
		double z_last;
		double d_factor;
		double h_factor;
		double amp;
	} d;
} emphasis_t;

#define CUT_OFF_EMPHASIS_DEFAULT	300.0

int init_emphasis(emphasis_t *state, int samplerate, double cut_off);
void pre_emphasis(emphasis_t *state, double *samples, int num);
void de_emphasis(emphasis_t *state, double *samples, int num);

