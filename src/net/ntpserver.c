/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "crypto/md5.h"
#include "crypto/sha.h"
#include "eeprom.h"
#include "lwip/udp.h"
#include "status.h"
#include "vtimer.h"
#include "net/ntpserver.h"
#include <string.h>


static void
ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port) {
	char *buf;
	uint64_t now;
	uint8_t md[20];
	int do_md, out_size;
	union {
		SHA_CTX sha;
		MD5_CTX md5;
	} ctx;

	buf = p->payload;
	if (p->len < 48 || (buf[0] & MODE_MASK) != MODE_CLIENT) {
		pbuf_free(p);
		return;
	}
	do_md = 0;
	out_size = 48;
	if (p->len == 72 && (cfg.flags & FLAG_NTPKEY_SHA1)) {
		SHA1_Init(&ctx.sha);
		SHA1_Update(&ctx.sha, &cfg.ntp_key, 20);
		SHA1_Update(&ctx.sha, buf, 48);
		SHA1_Final(md, &ctx.sha);
		if (!memcmp(md, &buf[52], 20)) {
			out_size = 72;
			do_md = 1;
		}
	} else if (p->len == 68 && (cfg.flags & FLAG_NTPKEY_MD5)) {
		MD5_Init(&ctx.md5);
		MD5_Update(&ctx.md5, &cfg.ntp_key, 20);
		MD5_Update(&ctx.md5, buf, 48);
		MD5_Final(md, &ctx.md5);
		if (!memcmp(md, &buf[52], 16)) {
			out_size = 68;
			do_md = 1;
		}
	}
	if (p->len > out_size) {
		pbuf_realloc(p, out_size);
	}

	/* FIXME: leap seconds */
	buf[0] = LEAP_NONE | VN_4 | MODE_SERVER;
	if (~status_flags & STATUS_READY) {
		/* not synced, advertise as such */
		buf[1] = 16;
	} else {
		/* operating normally */
		buf[1] = 1;
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
	if (do_md) {
		/* Leave keyid from the client */
		if (cfg.flags & FLAG_NTPKEY_SHA1) {
			SHA1_Init(&ctx.sha);
			SHA1_Update(&ctx.sha, &cfg.ntp_key, 20);
			SHA1_Update(&ctx.sha, buf, 48);
			SHA1_Final((uint8_t*)&buf[52], &ctx.sha);
		} else if (cfg.flags & FLAG_NTPKEY_MD5) {
			MD5_Init(&ctx.md5);
			MD5_Update(&ctx.md5, &cfg.ntp_key, 20);
			MD5_Update(&ctx.md5, buf, 48);
			MD5_Final((uint8_t*)&buf[52], &ctx.md5);
		}
	}

	udp_sendto(pcb, p, addr, port);
	pbuf_free(p);
}

struct udp_pcb *ntp_pcb;

void
ntp_server_start(void) {
	ASSERT((ntp_pcb = udp_new()) != NULL);
	udp_bind(ntp_pcb, IP_ADDR_ANY, NTP_PORT);
	udp_recv(ntp_pcb, ntp_recv, NULL);
}
