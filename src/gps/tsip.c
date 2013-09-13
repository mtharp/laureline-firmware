/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "vtimer.h"
#include "gps/parser.h"

static uint8_t rx_count;
static uint8_t rx_dle_count;
static uint64_t rx_last_tick;

/* Packet 0x8F-AB */
#define TFLAG_UTC			0x01
#define TFLAG_UTC_PPS		0x02
#define TFLAG_NO_TIME		0x04
#define TFLAG_NO_UTC		0x08
#define TFLAG_USER_TIME		0x10


static void
set_quant(uint8_t *qf) {
	float out_f;
	uint8_t *out_p = (uint8_t*)&out_f;
	/* Byte-swap IEEE 754 single */
	out_p[0] = qf[3];
	out_p[1] = qf[2];
	out_p[2] = qf[1];
	out_p[3] = qf[0];
	/* Value is in nanoseconds */
	out_f *= -(1 / 1e9f);
	vtimer_set_correction(out_f);
}


uint8_t
tsip_feed(uint8_t val) {
	if (CoGetOSTime() - rx_last_tick > MS2ST(50)) {
		// No bytes for 50ms, assume synchronization was lost somehow
		rx_count = rx_dle_count = 0;
	}
	rx_last_tick = CoGetOSTime();

	if (val == 0x10) {
		// "DLE" token
		rx_dle_count++;
		if (rx_dle_count > 2 && rx_dle_count % 2 == 1) {
			// Doubled-up (stuffed) DLE in the data part
			return FEED_CONTINUE;
		}
	} else if (rx_dle_count == 0) {
		// No start of packet seen yet, wait for DLE
		return FEED_UNKNOWN;
	}
	if (rx_count >= sizeof(pbuf)) {
		rx_count = rx_dle_count = 0;
		return FEED_UNKNOWN;
	}
	pbuf[rx_count++] = val;
	if (val == 0x03 && rx_dle_count % 2 == 0) {
		// End of packet
		if (rx_count >= 19 && pbuf[1] == 0x8f && pbuf[2] == 0xab) {
			if (pbuf[11] & (TFLAG_NO_TIME | TFLAG_NO_UTC)) {
				/* Time is incomplete */
				return FEED_COMPLETE;
			}
			/* FIXME: leap second */
			vtimer_set_utc(
					(pbuf[17] << 8) | pbuf[18],	/* year */
					pbuf[16],						/* month */
					pbuf[15],						/* day */
					pbuf[14],						/* hour */
					pbuf[13],						/* minute */
					pbuf[12],						/* second */
					0);								/* leap */
		} else if (rx_count > 65 && pbuf[1] == 0x8f && pbuf[2] == 0xac) {
			set_quant(&pbuf[62]);
		}
		rx_count = rx_dle_count = 0;
		return FEED_COMPLETE;
	}
	return FEED_CONTINUE;
}

