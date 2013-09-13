/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "serial.h"
#include "vtimer.h"
#include "gps/parser.h"

#define FEED_UNKNOWN 0
#define FEED_CONTINUE 1
#define FEED_COMPLETE 2

static enum {
	WAITING,
	SYNC,
	HEADER,
	COPYING,
	CHECKSUM1,
	CHECKSUM2
} ustate;

static uint8_t rx_count, rx_ck1, rx_ck2;
static uint16_t rx_pktlen;

#define TIMEUTC_VALIDTOW		0x01
#define TIMEUTC_VALIDWKN		0x02
#define TIMEUTC_VALIDUTC		0x04

const uint8_t ublox_cfg[] = {
	/* Enable NAV-TIMEUTC 0x01 0x21 */
	0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, 0x21,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x36, 0xA6,

	/* Enable TIM-TP 0x0D 0x01 */
	0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x0D, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x22, 0x26,
};



static void
set_quant_ubx(uint8_t *qf) {
	int32_t picoseconds = *(int32_t*)qf;
	float corr = -picoseconds * (1 / 1e12);
	vtimer_set_correction(corr);
}


uint8_t
ublox_feed(uint8_t val) {
	switch (ustate) {
	case WAITING:
		if (val == 0xB5) {
			ustate = SYNC;
			return FEED_CONTINUE;
		}
		break;
	case SYNC:
		if (val == 0x62) {
			rx_count = 0;
			rx_ck1 = 0;
			rx_ck2 = 0;
			ustate = HEADER;
			return FEED_CONTINUE;
		}
		break;
	case HEADER:
		pbuf[rx_count++] = val;
		rx_ck1 += (uint8_t)val;
		rx_ck2 += rx_ck1;
		if (rx_count >= 4) {
			rx_pktlen = ((pbuf[3] << 8) | pbuf[2]) + 4;
			if (rx_count >= rx_pktlen) {
				ustate = CHECKSUM1;
			} else {
				ustate = COPYING;
			}
		}
		return FEED_CONTINUE;
	case COPYING:
		if (rx_count >= sizeof(pbuf)) {
			break;
		}
		pbuf[rx_count++] = val;
		rx_ck1 += (uint8_t)val;
		rx_ck2 += rx_ck1;
		if (rx_count >= rx_pktlen) {
			ustate = CHECKSUM1;
		}
		return FEED_CONTINUE;
	case CHECKSUM1:
		if (rx_ck1 != val) {
			break;
		}
		ustate = CHECKSUM2;
		return FEED_CONTINUE;
	case CHECKSUM2:
		if (rx_ck2 != val) {
			break;
		}
		ustate = WAITING;
		if (pbuf[0] == 0x01 && pbuf[1] == 0x21 && rx_count >= 20) {
			/* NAV-TIMEUTC */
			if (!(pbuf[4+19] & TIMEUTC_VALIDUTC)) {
				return FEED_COMPLETE;
			}
			/* FIXME: leap second */
			vtimer_set_utc(
					(pbuf[4+12] | (pbuf[4+13] << 8)),	/* year */
					pbuf[4+14],							/* month */
					pbuf[4+15],							/* day */
					pbuf[4+16],							/* hour */
					pbuf[4+17],							/* minute */
					pbuf[4+18],							/* second */
					0);									/* leap */
		} else if (pbuf[0] == 0x0D && pbuf[1] == 0x01 && rx_count >= 16) {
			/* TIM-TP */
			set_quant_ubx(&pbuf[4+8]);
		}
		return FEED_COMPLETE;
	}
	ustate = WAITING;
	return FEED_UNKNOWN;
}


void
ublox_configure(serial_t *ch) {
	serial_write(ch, (const char*)ublox_cfg, sizeof(ublox_cfg));
}
