/* UHD device access
 *
 * (C) 2017 by Andreas Eversberg <jolly@eversberg.eu>
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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <uhd.h>
#include <uhd/usrp/usrp.h>
#include "uhd.h"
#include "debug.h"

static uhd_usrp_handle		usrp = NULL;
static uhd_tx_streamer_handle	tx_streamer = NULL;
static uhd_rx_streamer_handle	rx_streamer = NULL;
static uhd_tx_metadata_handle	tx_metadata = NULL;
static uhd_rx_metadata_handle	rx_metadata = NULL;
static uhd_tune_request_t	tune_request;
static uhd_tune_result_t	tune_result;
static uhd_stream_args_t	stream_args;
static uhd_stream_cmd_t		stream_cmd;
static size_t tx_samps_per_buff, rx_samps_per_buff;
static double			samplerate;
static int			check_rate; /* flag to check sample rate matches time stamp increment */
static time_t			rx_time_secs = 0;
static double			rx_time_fract_sec = 0.0;
static time_t			tx_time_secs = 0;
static double			tx_time_fract_sec = 0.0;

int uhd_open(const char *device_args, double tx_frequency, double rx_frequency, double rate, double rx_gain, double tx_gain)
{
	uhd_error error;
	double got_frequency, got_rate, got_gain;
	size_t channel = 0;

	samplerate = rate;
	check_rate = 1;

#warning HACK
if (tx_frequency < 200000000) tx_frequency = 463000000, rx_frequency = 463000000;

	/* create USRP */
	PDEBUG(DUHD, DEBUG_INFO, "Creating USRP with args \"%s\"...\n", device_args);
	error = uhd_usrp_make(&usrp, device_args);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to create USRP\n");
		uhd_close();
		return -EIO;
	}

	/* create streamers */
	error = uhd_tx_streamer_make(&tx_streamer);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to create TX streamer\n");
		uhd_close();
		return -EIO;
	}
	error = uhd_rx_streamer_make(&rx_streamer);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to create RX streamer\n");
		uhd_close();
		return -EIO;
	}

	/* create rx metadata */
	error = uhd_rx_metadata_make(&rx_metadata);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to create RX metadata\n");
		uhd_close();
		return -EIO;
	}

	/* set rate */
	error = uhd_usrp_set_tx_rate(usrp, rate, channel);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to set TX rate to %.0f Hz\n", rate);
		uhd_close();
		return -EIO;
	}
	error = uhd_usrp_set_rx_rate(usrp, rate, channel);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to set RX rate to %.0f Hz\n", rate);
		uhd_close();
		return -EIO;
	}

	/* see what rate actually is */
	error = uhd_usrp_get_tx_rate(usrp, channel, &got_rate);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to get TX rate\n");
		uhd_close();
		return -EIO;
	}
	if (got_rate != rate) {
		PDEBUG(DUHD, DEBUG_ERROR, "Given TX rate %.0f Hz is not supported, try %0.f Hz\n", rate, got_rate);
		uhd_close();
		return -EINVAL;
	}
	error = uhd_usrp_get_rx_rate(usrp, channel, &got_rate);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to get RX rate\n");
		uhd_close();
		return -EIO;
	}
	if (got_rate != rate) {
		PDEBUG(DUHD, DEBUG_ERROR, "Given RX rate %.0f Hz is not supported, try %0.f Hz\n", rate, got_rate);
		uhd_close();
		return -EINVAL;
	}

	/* set gain */
	error = uhd_usrp_set_tx_gain(usrp, tx_gain, channel, "");
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to set TX gain to %.0f\n", tx_gain);
		uhd_close();
		return -EIO;
	}
	error = uhd_usrp_set_rx_gain(usrp, rx_gain, channel, "");
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to set RX gain to %.0f\n", rx_gain);
		uhd_close();
		return -EIO;
	}

	/* see what gain actually is */
	error = uhd_usrp_get_tx_gain(usrp, channel, "", &got_gain);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to get TX gain\n");
		uhd_close();
		return -EIO;
	}
	if (got_gain != tx_gain) {
		PDEBUG(DUHD, DEBUG_NOTICE, "Given TX gain %.0f is not supported, we use %0.f\n", tx_gain, got_gain);
		tx_gain = got_gain;
	}
	error = uhd_usrp_get_rx_gain(usrp, channel, "", &got_gain);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to get RX gain\n");
		uhd_close();
		return -EIO;
	}
	if (got_gain != rx_gain) {
		PDEBUG(DUHD, DEBUG_NOTICE, "Given RX gain %.3f is not supported, we use %.3f\n", rx_gain, got_gain);
		rx_gain = got_gain;
	}

	/* set frequency */
	memset(&tune_request, 0, sizeof(tune_request));
        tune_request.target_freq = tx_frequency;
        tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
        tune_request.dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
	error = uhd_usrp_set_tx_freq(usrp, &tune_request, channel, &tune_result);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to set TX rate to %.0f Hz\n", tx_frequency);
		uhd_close();
		return -EIO;
	}
        tune_request.target_freq = rx_frequency;
	error = uhd_usrp_set_rx_freq(usrp, &tune_request, channel, &tune_result);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to set RX rate to %.0f Hz\n", rx_frequency);
		uhd_close();
		return -EIO;
	}

	/* see what frequency actually is */
	error = uhd_usrp_get_tx_freq(usrp, channel, &got_frequency);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to get TX frequency\n");
		uhd_close();
		return -EIO;
	}
	if (got_frequency != tx_frequency) {
		PDEBUG(DUHD, DEBUG_ERROR, "Given TX frequency %.0f Hz is not supported, try %0.f Hz\n", tx_frequency, got_frequency);
		uhd_close();
		return -EINVAL;
	}
	error = uhd_usrp_get_rx_freq(usrp, channel, &got_frequency);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to get RX frequency\n");
		uhd_close();
		return -EIO;
	}
	if (got_frequency != rx_frequency) {
		PDEBUG(DUHD, DEBUG_ERROR, "Given RX frequency %.0f Hz is not supported, try %0.f Hz\n", rx_frequency, got_frequency);
		uhd_close();
		return -EINVAL;
	}

	/* set up streamer */
	memset(&stream_args, 0, sizeof(stream_args));
        stream_args.cpu_format = "fc32";
        stream_args.otw_format = "sc16";
        stream_args.args = "";
        stream_args.channel_list = &channel;
        stream_args.n_channels = 1;
	error = uhd_usrp_get_tx_stream(usrp, &stream_args, tx_streamer);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to set TX streamer args\n");
		uhd_close();
		return -EIO;
	}
	error = uhd_usrp_get_rx_stream(usrp, &stream_args, rx_streamer);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to set RX streamer args\n");
		uhd_close();
		return -EIO;
	}

	/* get buffer sizes */
	error = uhd_tx_streamer_max_num_samps(tx_streamer, &tx_samps_per_buff);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to get TX streamer sample buffer\n");
		uhd_close();
		return -EIO;
	}
	error = uhd_rx_streamer_max_num_samps(rx_streamer, &rx_samps_per_buff);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to get RX streamer sample buffer\n");
		uhd_close();
		return -EIO;
	}

	/* enable rx stream */
	memset(&stream_cmd, 0, sizeof(stream_cmd));
	stream_cmd.stream_mode = UHD_STREAM_MODE_START_CONTINUOUS;
	stream_cmd.stream_now = true;
	error = uhd_rx_streamer_issue_stream_cmd(rx_streamer, &stream_cmd);
	if (error) {
		PDEBUG(DUHD, DEBUG_ERROR, "Failed to issue RX stream command\n");
		uhd_close();
		return -EIO;
	}
	return 0;
}

void uhd_close(void)
{
	PDEBUG(DUHD, DEBUG_DEBUG, "Clean up UHD\n");
	if (tx_metadata)
        	uhd_tx_metadata_free(&tx_metadata);
	if (rx_metadata)
        	uhd_rx_metadata_free(&rx_metadata);
	if (tx_streamer)
	        uhd_tx_streamer_free(&tx_streamer);
	if (rx_streamer)
	        uhd_rx_streamer_free(&rx_streamer);
	if (usrp)
	        uhd_usrp_free(&usrp);
}

int uhd_send(float *buff, int num)
{
    	const void *buffs_ptr[1];
	int chunk;
	size_t sent = 0, count;
	uhd_error error;

	while (num) {
		chunk = num;
		if (chunk > (int)tx_samps_per_buff)
			chunk = (int)tx_samps_per_buff;
		/* create tx metadata */
		error = uhd_tx_metadata_make(&tx_metadata, true, tx_time_secs, tx_time_fract_sec, true, false);
		if (error)
			PDEBUG(DUHD, DEBUG_ERROR, "Failed to create TX metadata\n");
		buffs_ptr[0] = buff;
		count = 0;
		error = uhd_tx_streamer_send(tx_streamer, buffs_ptr, chunk, &tx_metadata, 0.0, &count);
		if (error) {
			PDEBUG(DUHD, DEBUG_ERROR, "Failed to write to TX streamer\n");
			break;
		}
		if (count == 0)
			break;

		/* increment time stamp */
		tx_time_fract_sec += (double)count / samplerate;
		while (tx_time_fract_sec >= 1.0) {
			tx_time_secs++;
			tx_time_fract_sec -= 1.0;
		}
//printf("adv=%.3f\n", ((double)tx_time_secs + tx_time_fract_sec) - ((double)rx_time_secs + rx_time_fract_sec));

		sent += count;
		buff += count * 2;
		num -= count;
	}

	return sent;
}

/* read what we got, return 0, if buffer is empty, otherwise return the number of samples */
int uhd_receive(float *buff, int max)
{
    	void *buffs_ptr[1];
	size_t got = 0, count;
	uhd_error error;
	time_t last_secs = rx_time_secs;
	double last_fract_sec = rx_time_fract_sec;

	if (max < (int)rx_samps_per_buff) {
		PDEBUG(DUHD, DEBUG_ERROR, "SDR rx buffer too small, please fix!\n");
		return 0;
	}

	while (max >= (int)rx_samps_per_buff) {
		buffs_ptr[0] = buff;
		count = 0;
		error = uhd_rx_streamer_recv(rx_streamer, buffs_ptr, rx_samps_per_buff, &rx_metadata, 0.0, false, &count);
		if (error) {
			PDEBUG(DUHD, DEBUG_ERROR, "Failed to read from UHD device.\n");
			break;
		}
		if (count == 0)
			break;

		uhd_rx_metadata_time_spec(rx_metadata, &rx_time_secs, &rx_time_fract_sec);
		/* check sample rate matches time stamp increment */
		if (last_secs || last_fract_sec) {
			double got = ((double)rx_time_secs + rx_time_fract_sec) - ((double)last_secs + last_fract_sec);
			double expect = (double)count / samplerate;
			double diff = fabs(got - expect);
			if (diff > 0.000000000001) {
				if (check_rate) {
					PDEBUG(DUHD, DEBUG_ERROR, "Received rate (%.0f) does not match defined rate (%.0f), use diffrent sample rate that UHD device can handle!\n", (double)count / got, samplerate);
					return -EPERM;
				}
				int gap = diff * (double)samplerate;
				PDEBUG(DUHD, DEBUG_ERROR, "Detected a gap of %.6f secods (%d samples), \n", diff, gap);
#warning fill gap
			}
			check_rate = 0;
		}
//		printf("s = %d time = %.12f samples = %.12f\n", count, ((double)rx_time_secs + rx_time_fract_sec) - ((double)last_s + last_f), (double)count / samplerate);

		got += count;
		buff += count * 2;
		max -= count;
	}

	return got;
}

/* estimate current unsent number of samples */
int uhd_get_inbuffer(void)
{
	double advance;

	/* we need the rx time stamp to determine how much data is already sent in advance */
	if (rx_time_secs == 0 && rx_time_fract_sec == 0.0)
		return -EAGAIN;

	/* if we have not yet sent any data, we set initial tx time stamp */
	if (tx_time_secs == 0 && tx_time_fract_sec == 0.0) {
		tx_time_secs = rx_time_secs;
		tx_time_fract_sec = rx_time_fract_sec;
	}

	/* we check how advance our transmitted time stamp is */
	advance = ((double)tx_time_secs + tx_time_fract_sec) - ((double)rx_time_secs + rx_time_fract_sec);
	/* in case of underrun: */
	if (advance < 0)
		advance = 0;

	return (int)(advance * samplerate);
}

