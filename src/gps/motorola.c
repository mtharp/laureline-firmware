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


enum mstate_e {
	waiting = 0,
	at1,
	at2,
	cmd1,
	copying
} mstate;
uint8_t mremaining, mcsum;
#define pbufSIZE		72
uint16_t mcmd;
uint8_t *mptr;

#define MCMD_Ea			0x4561


static const char wakeup[] = "@@Ea\1\x25\r\n"
								"@@En\0\1\x04\x00\x02\0\0\0\0\0\0\0\0\0\0\x2c\r\n";


uint8_t
motorola_feed(uint8_t data) {
	switch (mstate) {
	case waiting:
		if (data == '@') {
			mstate = at1;
			return FEED_CONTINUE;
		}
		return 0;
	case at1:
		if (data == '@') {
			mstate = at2;
		} else {
			mstate = waiting;
		}
		return FEED_CONTINUE;
	case at2:
		mcmd = data;
		mcsum = data;
		mstate = cmd1;
		return FEED_CONTINUE;
	case cmd1:
		mcmd = (mcmd << 8) | data;
		mcsum ^= data;
		switch (mcmd) {
		case MCMD_Ea: mremaining = 70; break;
		default:
			/* Unknown command */
			mstate = waiting;
			return FEED_UNKNOWN;
		}
		if (mremaining > PBUF_SIZE) {
			/* Packet too big */
			mstate = waiting;
			return FEED_UNKNOWN;
		}
		mstate = copying;
		mptr = &pbuf[0];
		return FEED_CONTINUE;
	case copying:
		*mptr++ = data;
		mcsum ^= data;
		if (--mremaining != 0) {
			/* Still copying */
			return FEED_CONTINUE;
		}
		mstate = waiting;
		if (mcsum != 0) {
			/* Bad checksum */
			return FEED_UNKNOWN;
		}
		switch (mcmd) {
		case MCMD_Ea:
			/* FIXME: leap second */
			vtimer_set_utc(
					(pbuf[2] << 8) | pbuf[3],	/* year */
					pbuf[0],					/* month */
					pbuf[1],					/* day */
					pbuf[4],					/* hour */
					pbuf[5],					/* minute */
					pbuf[6],					/* second */
					0);							/* leap */
		}
		return FEED_COMPLETE;
	}
	return FEED_UNKNOWN;
}
