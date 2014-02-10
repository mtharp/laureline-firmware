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
#include "net/ntpclient.h"
#include "net/ntpserver.h"
#include <string.h>

#define MAX_SERVERS 4

#if 0

struct udp_pcb *ntp_cli_pcb;
static uint64_t next_query, next_timeout, next_dns;
static uint32_t ntp_servers[MAX_SERVERS];
static uint64_t ntp_responses[MAX_SERVERS];

static void
ntp_cli_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port) {
	char *buf;
	uint64_t now;

	buf = p->payload;
	if (p->len < 48
			|| (buf[0] & MODE_MASK) != MODE_SERVER
			|| buf[1] == 0 || buf[1] > 15 /* stratum */
			) {
		pbuf_free(p);
		return;
	}

	now =	( (uint64_t)buf[40] << 56
			| (uint64_t)buf[41] << 48
			| (uint64_t)buf[42] << 40
			| (uint64_t)buf[43] << 32
			| (uint64_t)buf[44] << 24
			| (uint64_t)buf[45] << 16
			| (uint64_t)buf[46] <<  8
			| (uint64_t)buf[47]);

	pbuf_free(p);
}


static void
ntp_start_query(void) {
}

void
ntp_client_start(void) {
	ntp_cli_pcb = udp_new();
	udp_bind(ntp_cli_pcb, IP_ADDR_ANY, 0);
	udp_recv(ntp_cli_pcb, ntp_cli_recv, NULL);
}


void
ntp_client_periodic(void) {
	uint64_t now = vtimer_now();
	if (next_timeout != 0 && now > next_timeout) {
		ntp_finish_query();
	}
	if (next_timeout == 0 && now > next_query) {
		ntp_start_query();
	}
}
#endif
