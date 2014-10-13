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

#pragma pack(push, 1)
struct ntp_msg {
    uint8_t mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint32_t ref_ts[2];
    uint32_t origin_ts[2];
    uint32_t rx_ts[2];
    uint32_t tx_ts[2];
    /* Optional below here */
    uint32_t key_id;
    uint8_t digest[20];
};
#pragma pack(pop)


static void
ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port) {
    struct ntp_msg *msg;
    uint64_t now;
    uint8_t md[20];
    int out_size;

    msg = (struct ntp_msg*)p->payload;
    if (p->len < 48 || (msg->mode & MODE_MASK) != MODE_CLIENT) {
        pbuf_free(p);
        return;
    }
    out_size = 48;
    if (p->len == 72 && (cfg.flags & FLAG_NTPKEY_SHA1)) {
        SHA_CTX sha;
        SHA1_Init(&sha);
        SHA1_Update(&sha, &cfg.ntp_key, 20);
        SHA1_Update(&sha, msg, 48);
        SHA1_Final(md, &sha);
        if (!memcmp(md, msg->digest, 20)) {
            out_size = 72;
        }
    } else if (p->len == 68 && (cfg.flags & FLAG_NTPKEY_MD5)) {
        MD5_CTX md5;
        MD5_Init(&md5);
        MD5_Update(&md5, &cfg.ntp_key, 20);
        MD5_Update(&md5, msg, 48);
        MD5_Final(md, &md5);
        if (!memcmp(md, msg->digest, 16)) {
            out_size = 68;
        }
    }
    if (p->len > out_size) {
        pbuf_realloc(p, out_size);
    }

    /* FIXME: leap seconds */
    msg->mode = LEAP_NONE | VN_4 | MODE_SERVER;
    if (~status_flags & STATUS_READY) {
        /* not synced, advertise as such */
        msg->stratum = 16;
    } else {
        /* operating normally */
        msg->stratum = 1;
    }
    if (msg->poll < 6) {
        msg->poll = 6;
    }
    msg->precision = -24; /* 59ns */
    msg->root_delay = 0;
    msg->root_dispersion = 0;
    msg->ref_id = PP_HTONL(0x47505300); /* GPS */
    /* copy transmit to origin */
    msg->origin_ts[0] = msg->tx_ts[0];
    msg->origin_ts[1] = msg->tx_ts[1];

    /* Write current time into reference and receive fields. The transmit field
     * and (optional) digest will be added right before the packet goes out, in
     * ntp_finish().
     */
    now = vtimer_now();
    msg->ref_ts[0] = msg->rx_ts[0] = PP_HTONL((now >> 32));
    msg->ref_ts[1] = msg->rx_ts[1] = PP_HTONL(now);

    p->flags |= PBUF_FLAG_NTP;
    udp_sendto(pcb, p, addr, port);
    pbuf_free(p);
}


void
ntp_finish(struct pbuf *p) {
    struct ntp_msg *msg;
    uint64_t now;

    ASSERT(p->len >= 48);
    msg = (struct ntp_msg*)p->payload;
    now = vtimer_now();
    msg->tx_ts[0] = PP_HTONL((now >> 32));
    msg->tx_ts[1] = PP_HTONL(now);
    if (p->len > 48) {
        if (cfg.flags & FLAG_NTPKEY_SHA1) {
            SHA_CTX sha;
            ASSERT(p->len == 72);
            SHA1_Init(&sha);
            SHA1_Update(&sha, &cfg.ntp_key, 20);
            SHA1_Update(&sha, msg, 48);
            SHA1_Final(msg->digest, &sha);
        } else if (cfg.flags & FLAG_NTPKEY_MD5) {
            MD5_CTX md5;
            ASSERT(p->len == 68);
            MD5_Init(&md5);
            MD5_Update(&md5, &cfg.ntp_key, 20);
            MD5_Update(&md5, msg, 48);
            MD5_Final(msg->digest, &md5);
        } else {
            ASSERT(0);
        }
    }
}


struct udp_pcb *ntp_pcb;
#if LWIP_IPV6
struct udp_pcb *ntp6_pcb;
#endif

void
ntp_server_start(void) {
    ASSERT((ntp_pcb = udp_new()) != NULL);
    udp_bind(ntp_pcb, IP_ADDR_ANY, NTP_PORT);
    udp_recv(ntp_pcb, ntp_recv, NULL);
#if LWIP_IPV6
    ASSERT((ntp6_pcb = udp_new_ip6()) != NULL);
    udp_bind_ip6(ntp6_pcb, IP6_ADDR_ANY, NTP_PORT);
    udp_recv(ntp6_pcb, ntp_recv, NULL);
#endif
}
