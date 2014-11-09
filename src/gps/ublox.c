/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "task.h"

#include "gps/ublox.h"
#include "cmdline.h"
#include "eeprom.h"
#include "logging.h"
#include "main.h"
#include "vtimer.h"
#include "gps/parser.h"
#include "stm32/serial.h"
#include <string.h>

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

static const uint8_t ublox_cfg[] = {
    /*  msg   | interval */
    0x01, 0x06, 0x01, /* NAV-SOL */
    0x01, 0x20, 0x05, /* NAV-TIMEGPS */
    0x01, 0x21, 0x01, /* NAV-TIMEUTC */
    0x0D, 0x01, 0x01, /* TIM-TP */
};


static const uint8_t ublox_startup[] = {
    0xB5, 0x62, 0x0A, 0x04, 0x00, 0x00, 0x0E, 0x34, /* MON-VER */
};
static const uint8_t ublox_hui_req[] = {
    0xB5, 0x62, 0x0B, 0x02, 0x00, 0x00, 0x0D, 0x32,
};


static void
set_quant_ubx(uint8_t *qf) {
    int32_t picoseconds = *(int32_t*)qf;
    float corr = -picoseconds * (1 / 1e12);
    vtimer_set_correction(corr, LEADING);
}


uint8_t
ublox_feed(uint8_t val) {
    static uint8_t rx_count, rx_ck1, rx_ck2;
    static uint16_t rx_pktlen;
    static uint64_t last_hui;

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
        if (pbuf[0] == 0x01 && pbuf[1] == 0x06 && rx_count >= 4+52) {
            /* NAV-SOL */
            gps_fix_svs = pbuf[4+47];
        } else if (pbuf[0] == 0x01 && pbuf[1] == 0x20 && rx_count >= 4+16) {
            /* NAV-TIMEGPS */
            nav_timegps_t *msg = (nav_timegps_t *)pbuf;
            if ((cfg.flags & FLAG_TIMESCALE_GPS)
                    && (msg->valid & TIMEUTC_VALIDWKN)
                    && (msg->valid & TIMEUTC_VALIDTOW)) {
                vtimer_set_gps(msg->week, msg->iTOW / 1000);
            }
        } else if (pbuf[0] == 0x01 && pbuf[1] == 0x21 && rx_count >= 4+20) {
            /* NAV-TIMEUTC */
            nav_timeutc_t *msg = (nav_timeutc_t *)pbuf;
            if (!(cfg.flags & FLAG_TIMESCALE_GPS)
                    && (msg->valid & TIMEUTC_VALIDUTC)) {
                /* FIXME: leap second */
                vtimer_set_utc(msg->year, msg->month, msg->day, msg->hour,
                        msg->min, msg->sec);
            }
        } else if (pbuf[0] == 0x0B && pbuf[1] == 0x02 && rx_count >= 4+72) {
            /* AID-HUI */
        } else if (pbuf[0] == 0x0D && pbuf[1] == 0x01 && rx_count >= 4+16) {
            /* TIM-TP */
            set_quant_ubx(&pbuf[4+8]);
        }
        return FEED_COMPLETE;
    }
    ustate = WAITING;
    return FEED_UNKNOWN;
}


void
ublox_configure(void) {
    static const uint8_t ublox_template[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00 };
    const uint8_t *x;
    uint8_t buf[16], ck1, ck2, i;
    memset(buf, 0, 14);
    memcpy(buf, ublox_template, 6);
    for (x = ublox_cfg; x < ublox_cfg + sizeof(ublox_cfg); x += 3) {
        buf[6] = x[0];
        buf[7] = x[1];
        buf[9] = x[2];
        ck1 = ck2 = 0;
        for (i = 2; i < 14; i++) {
            ck1 += buf[i];
            ck2 += ck1;
        }
        buf[14] = ck1;
        buf[15] = ck2;
        serial_write(gps_serial, (const char *)buf, 16);
    }
}
