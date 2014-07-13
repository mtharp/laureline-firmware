/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "net/relay.h"
#include "ppscapture.h"
#include "gps/motorola.h"
#include "gps/nmea.h"
#include "gps/parser.h"
#include "gps/tsip.h"
#include "gps/ublox.h"

uint8_t pbuf[PBUF_SIZE];
int gps_fix_svs;

static uint8_t current_proto, last_proto;
static uint64_t time_last_byte, time_last_packet;
static const struct parserdecl *parser;

const struct parserdecl {
	uint8_t proto;
	uint8_t (*func)(uint8_t);
} parsers[] = {
	{PROTO_UBLOX, ublox_feed},
	{PROTO_TSIP, tsip_feed},
	{PROTO_ONCORE, motorola_feed},
	{PROTO_NMEA, nmea_feed},
	{0, NULL}};


void
gps_byte_received(uint8_t data) {
	uint8_t rc;
	if (CoGetOSTime() - time_last_byte >= PACKET_TIMEOUT) {
		/* Clear the current parser if there is an interpacket gap. */
		current_proto = PROTO_NONE;
		/* Flush the broadcast buffer */
		relay_flush();
	}
	time_last_byte = CoGetOSTime();
	relay_push(data);

	if (CoGetOSTime() - time_last_packet > PARSER_TIMEOUT) {
		/* Clear out the parser state if nothing is recognized for
		 * a few seconds
		 */
		current_proto = PROTO_NONE;
		last_proto = PROTO_NONE;
		gps_fix_svs = 0;
	}
	for (parser = &parsers[0]; parser->func; parser++) {
		if (current_proto != PROTO_NONE && current_proto != parser->proto) {
			continue;
		}
		rc = parser->func(data);
		if (rc == FEED_CONTINUE) {
			/* Lock out all other parsers */
			current_proto = parser->proto;
			break;
		} else if (rc == FEED_COMPLETE) {
			/* Go back to search mode */
			current_proto = PROTO_NONE;
			last_proto = parser->proto;
			time_last_packet = CoGetOSTime();
			relay_flush();
			break;
		}
	}
}
