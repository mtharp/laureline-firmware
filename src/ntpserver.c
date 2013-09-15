/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "lwip/udp.h"
#include "status.h"
#include "vtimer.h"
#include <string.h>

#define LEAP_MASK				0xC0
#define LEAP_NONE				0x0
#define LEAP_INSERT				0x1
#define LEAP_DELETE				0x2
#define LEAP_UNKNOWN			0x3
#define VN_MASK					0x38
#define VN_4					(4 << 3)
#define MODE_MASK				0x07
#define MODE_CLIENT				0x3
#define MODE_SERVER				0x4

#define TICKS_TO_NTP			263524915339ULL



static void
ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port) {
	char *buf;
	uint64_t now;

	buf = p->payload;
	if (p->len < 48 || (buf[0] & MODE_MASK) != MODE_CLIENT) {
		pbuf_free(p);
		return;
	}
	if (p->len > 48) {
		pbuf_realloc(p, 48);
	}

	/* FIXME: leap seconds */
	buf[0] = LEAP_NONE | VN_4 | MODE_SERVER;
	if (isSettled()) {
		/* operating normally */
		buf[1] = 1;
	} else {
		/* not synced, advertise as such */
		buf[1] = 16;
	}
	if (buf[2] < 6) {
		/* Minimum poll interval of 64s */
		buf[2] = 6;
	}
	/* Leave poll interval as the client specified */
	buf[3] = -24;							/* precision -  59ns */
	buf[4] = buf[5] = buf[6] = buf[7] = 0;				/* root delay */
	buf[8] = 0; buf[9] = 0; buf[10] = 0; buf[11] = 0;		/* root dispersion */
	buf[12] = 'G'; buf[13] = 'P', buf[14] = 'S', buf[15] = 0;	/* refid */
	memcpy(&buf[24], &buf[40], 8);					/* copy transmit to origin */

	/* Write current time into reference and transmit fields */
	now = vtimer_now();
	buf[16] = now >> 56;
	buf[17] = now >> 48;
	buf[18] = now >> 40;
	buf[19] = now >> 32;
	buf[20] = now >> 24;
	buf[21] = now >> 16;
	buf[22] = now >> 8;
	buf[23] = now;
	memcpy(&buf[32], &buf[16], 8);
	memcpy(&buf[40], &buf[16], 8);

	udp_sendto(pcb, p, addr, port);
	pbuf_free(p);
}

struct udp_pcb *ntp_pcb;

void
ntp_server_start(void) {
	ntp_pcb = udp_new();
	udp_bind(ntp_pcb, IP_ADDR_ANY, 123);
	udp_recv(ntp_pcb, ntp_recv, NULL);
}
