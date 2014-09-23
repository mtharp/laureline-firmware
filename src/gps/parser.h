/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _GPS_PARSER_H
#define _GPS_PARSER_H

#define PACKET_TIMEOUT      MS2ST(10)
#define PARSER_TIMEOUT      S2ST(5)

/* Protocols that might be detected */
#define PROTO_NONE      0
#define PROTO_NMEA      1
#define PROTO_TSIP      2
#define PROTO_TEP       3
#define PROTO_ONCORE    4
#define PROTO_UBLOX     5

/* Status codes for feed_* return */
#define FEED_UNKNOWN        0
#define FEED_CONTINUE       1
#define FEED_COMPLETE       2

#define PBUF_SIZE           128
extern uint8_t pbuf[PBUF_SIZE];

extern int gps_fix_svs;

void gps_byte_received(uint8_t data);

#endif
