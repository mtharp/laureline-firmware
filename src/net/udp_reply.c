/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "lwip/snmp.h"
#include "lwip/stats.h"
#include "lwip/udp.h"
#include "netif/etharp.h"
#include <string.h>


/* Reply to a UDP packet without doing an ARP or ND6 lookup. Just exchange the
 * layer 2 addresses. This makes a lot of assumptions about configuration.
 */
err_t
udp_reply(struct udp_pcb *pcb, struct pbuf *p, struct netif *netif) {
    /* UDP (layer 4) */
    {
        struct udp_hdr *udphdr;
        uint16_t port_tmp;
        ASSERT(!pbuf_header(p, UDP_HLEN));
        udphdr = (struct udp_hdr *)p->payload;
        port_tmp = udphdr->src;
        udphdr->src = udphdr->dest;
        udphdr->dest = port_tmp;
        udphdr->chksum = 0x0000;
        udphdr->len = htons(p->tot_len);
    }

    /* IP (layer 3) */
    if (PCB_ISIPV6(pcb)) {
        struct ip6_hdr *ip6hdr;
        ASSERT(!pbuf_header(p, IP6_HLEN));
        ip6hdr = (struct ip6_hdr *)p->payload;
        IP6H_VTCFL_SET(ip6hdr, 6, IP6H_TC(ip6hdr), 0);
        IP6H_PLEN_SET(ip6hdr, p->tot_len - IP6_HLEN);
        IP6H_NEXTH_SET(ip6hdr, IP6_NEXTH_UDP);
        IP6H_HOPLIM_SET(ip6hdr, pcb->ttl);
        ip6_addr_copy(ip6hdr->dest, ip6hdr->src);
        ip6_addr_copy(ip6hdr->src,
                *ip6_select_source_address(netif, (ip6_addr_t *)&ip6hdr->dest));
        IP6_STATS_INC(ip6.xmit);
    } else {
        struct ip_hdr *iphdr;
        ASSERT(!pbuf_header(p, IP_HLEN));
        iphdr = (struct ip_hdr *)p->payload;
        IPH_TTL_SET(iphdr, pcb->ttl);
        IPH_PROTO_SET(iphdr, IP_PROTO_UDP);
        ip_addr_copy(iphdr->dest, iphdr->src);
        ip_addr_copy(iphdr->src, netif->ip_addr);
        IPH_VHL_SET(iphdr, 4, IP_HLEN / 4);
        /* retain id and tos fields */
        IPH_LEN_SET(iphdr, htons(p->tot_len));
        IPH_OFFSET_SET(iphdr, 0);
        IPH_CHKSUM_SET(iphdr, 0);
        IP_STATS_INC(ip.xmit);
    }

    /* Ethernet (layer 2) */
    {
        struct eth_hdr *ethhdr;
        struct eth_addr dest;
        ASSERT(!pbuf_header(p, SIZEOF_ETH_HDR));
        ethhdr = (struct eth_hdr *)p->payload;
        ethhdr->type = PP_HTONS(PCB_ISIPV6(pcb)
                ? ETHTYPE_IPV6 : ETHTYPE_IP);
        ETHADDR16_COPY(&dest, &ethhdr->src);
        ETHADDR16_COPY(&ethhdr->src, netif->hwaddr);
        ETHADDR32_COPY(&ethhdr->dest, &dest);
    }

    snmp_inc_udpoutdatagrams();
    return netif->linkoutput(netif, p);
}
