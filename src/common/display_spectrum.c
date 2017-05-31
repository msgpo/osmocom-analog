/* display spectrum of IQ data
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
#include <math.h>
#include "sample.h"
#include "sender.h"
#include "fft.h"

#define HEIGHT	20

static double buffer_max[MAX_DISPLAY_SPECTRUM];
static char screen[HEIGHT][MAX_DISPLAY_WIDTH];
static uint8_t screen_color[HEIGHT][MAX_DISPLAY_WIDTH];
static int spectrum_on = 0;
static double db = 100;

static dispspectrum_t disp;

void display_spectrum_init(int samplerate)
{
	memset(&disp, 0, sizeof(disp));
	disp.interval_max = (double)samplerate * DISPLAY_INTERVAL + 0.5;
	/* should not happen due to low interval */
	if (disp.interval_max < MAX_DISPLAY_SPECTRUM - 1)
		disp.interval_max = MAX_DISPLAY_SPECTRUM - 1;
	memset(buffer_max, 0, sizeof(buffer_max));
}

void display_spectrum_on(int on)
{
	int j;
	int w, h;

	get_win_size(&w, &h);

	if (spectrum_on) {
		memset(&screen, ' ', sizeof(screen));
		printf("\0337\033[H");
		for (j = 0; j < HEIGHT; j++) {
			screen[j][w] = '\0';
			puts(screen[j]);
		}
		printf("\0338"); fflush(stdout);
	}

	if (on < 0) {
		if (++spectrum_on == 2)
			spectrum_on = 0;
	} else
		spectrum_on = on;
}

void display_spectrum_limit_scroll(int on)
{
	int w, h;

	if (!spectrum_on)
		return;

	get_win_size(&w, &h);

	printf("\0337");
	printf("\033[%d;%dr", (on) ? HEIGHT + 1 : 1, h);
	printf("\0338");
}

/*
 * plot spectrum data:
 *
 */
void display_spectrum(float *samples, int length)
{
	int width, h;
	int pos, max;
	double *buffer_I, *buffer_Q;
	int color = 9; /* default color */
	int i, j, k, o;
	double I, Q, v;
	int s, e;

	if (!spectrum_on)
		return;

	get_win_size(&width, &h);
	if (width > MAX_DISPLAY_WIDTH)
		width = MAX_DISPLAY_WIDTH;

	/* calculate size of FFT */
	int m, fft_size = 0, fft_taps = 0;
	for (m = 0; m < 16; m++) {
		if ((1 << m) > MAX_DISPLAY_SPECTRUM)
			break;
		if ((1 << m) <= width) {
			fft_taps = m;
			fft_size = 1 << m;
		}
	}
	if (m == 16) {
		fprintf(stderr, "Size of spectrum is not a power of 2, please fix!\n");
		abort();
	}

	int heigh[fft_size], low[fft_size];

	pos = disp.interval_pos;
	max = disp.interval_max;
	buffer_I = disp.buffer_I;
	buffer_Q = disp.buffer_Q;

	for (i = 0; i < length; i++) {
		if (pos >= fft_size) {
			if (++pos == max)
				pos = 0;
			continue;
		}
		buffer_I[pos] = samples[i * 2];
		buffer_Q[pos] = samples[i * 2 + 1];
		pos++;
		if (pos == fft_size) {
			fft_process(1, fft_taps, buffer_I, buffer_Q);
			k = 0;
			for (j = 0; j < fft_size; j++) {
				/* scale result vertically */
				I = buffer_I[(j + fft_size / 2) % fft_size];
				Q = buffer_Q[(j + fft_size / 2) % fft_size];
				v = sqrt(I*I + Q*Q);
				v = log10(v) * 20 + db;
				if (v < 0)
					v = 0;
				v /= db;
				buffer_max[j] -= DISPLAY_INTERVAL / 10.0;
				if (v > buffer_max[j])
					buffer_max[j] = v;
				
				/* heigh is the maximum value */
				heigh[j] = (double)(HEIGHT * 2 - 1) * (1.0 - buffer_max[j]);
				if (heigh[j] < 0)
					heigh[j] = 0;
				if (heigh[j] >= (HEIGHT * 2))
					heigh[j] = (HEIGHT * 2) - 1;
				/* low is the current value */
				low[j] = (double)(HEIGHT * 2 - 1) * (1.0 - v);
				if (low[j] < 0)
					low[j] = 0;
				if (low[j] >= (HEIGHT * 2))
					low[j] = (HEIGHT * 2) - 1;
			}
			/* plot scaled buffer */
			memset(&screen, ' ', sizeof(screen));
			memset(&screen_color, 7, sizeof(screen_color)); /* all white */
			sprintf(screen[0], "(spectrum log %.0f dB", db);
			*strchr(screen[0], '\0') = ')';
			o = (width - fft_size) / 2; /* offset from left border */
			for (j = 0; j < fft_size; j++) {
				s = e = low[j];
				if (j > 0 && low[j - 1] > e)
					e = low[j - 1] - 1;
				if (j < fft_size - 1 && low[j + 1] > e)
					e = low[j + 1] - 1;
				if (s == e) {
					if ((s & 1) == 0)
						screen[s >> 1][j + o] = '\'';
					else
						screen[s >> 1][j + o] = '.';
					screen_color[s >> 1][j + o] = 13;
				} else {
					if ((s & 1) == 0)
						screen[s >> 1][j + o] = '|';
					else
						screen[s >> 1][j + o] = '.';
					screen_color[s >> 1][j + o] = 13;
					if ((e & 1) == 0)
						screen[e >> 1][j + o] = '\'';
					else
						screen[e >> 1][j + o] = '|';
					screen_color[e >> 1][j + o] = 13;
					for (k = (s >> 1) + 1; k < (e >> 1); k++) {
						screen[k][j + o] = '|';
						screen_color[k][j + o] = 13;
					}
				}
				s = heigh[j];
				e = low[j];
				if ((s >> 1) < (e >> 1)) {
					if ((s & 1) == 0)
						screen[s >> 1][j + o] = '|';
					else
						screen[s >> 1][j + o] = '.';
					screen_color[s >> 1][j + o] = 4;
					for (k = (s >> 1) + 1; k < (e >> 1); k++) {
						screen[k][j + o] = '|';
						screen_color[k][j + o] = 4;
					}
				}
			}
			printf("\0337\033[H");
			for (j = 0; j < HEIGHT; j++) {
				for (k = 0; k < width; k++) {
					if (screen_color[j][k] != color) {
						color = screen_color[j][k];
						printf("\033[%d;3%dm", color / 10, color % 10);
					}
					putchar(screen[j][k]);
				}
				printf("\n");
			}
			/* reset color and position */
			printf("\033[0;39m\0338"); fflush(stdout);
		}
	}

	disp.interval_pos = pos;
}


