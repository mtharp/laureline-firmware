/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef __LWIPOPT_H__
#define __LWIPOPT_H__

#include "arch/sys_arch.h"

/*
   -----------------------------------------------
   ---------- Platform specific locking ----------
   -----------------------------------------------
*/

#define NO_SYS                          1
#define NO_SYS_NO_TIMERS                0

/*
   ------------------------------------
   ---------- Memory options ----------
   ------------------------------------
*/
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        1600
#ifdef DEBUG
# define MEMP_OVERFLOW_CHECK			1
# define MEMP_SANITY_CHECK				1
#else
# define MEMP_OVERFLOW_CHECK			0
# define MEMP_SANITY_CHECK				0
#endif

/*
   ------------------------------------------------
   ---------- Internal Memory Pool Sizes ----------
   ------------------------------------------------
*/
#define MEMP_NUM_PBUF					4
#define MEMP_NUM_UDP_PCB				4
#define MEMP_NUM_TCP_PCB				8
#define MEMP_NUM_TCP_PCB_LISTEN			2
#define MEMP_NUM_TCP_SEG				16
#define PBUF_POOL_SIZE					4

/*
   ---------------------------------
   ---------- ARP options ----------
   ---------------------------------
*/
#define LWIP_ARP                        1
#define ARP_TABLE_SIZE                  4
#define ARP_QUEUEING                    0
#define ETHARP_TRUST_IP_MAC             1
#define ETH_PAD_SIZE                    2

/*
   --------------------------------
   ---------- IP options ----------
   --------------------------------
*/
#define IP_REASSEMBLY                   0
#define IP_FRAG                         0

/*
   ----------------------------------
   ---------- ICMP options ----------
   ----------------------------------
*/
#define LWIP_ICMP                       1

/*
   ----------------------------------
   ---------- DHCP options ----------
   ----------------------------------
*/
#define LWIP_DHCP                       1
#define DHCP_DOES_ARP_CHECK             1

/*
   ---------------------------------
   ---------- UDP options ----------
   ---------------------------------
*/
#define LWIP_UDP                        1
#define LWIP_UDPLITE                    0
#define LWIP_NETBUF_RECVINFO            0

/*
   ---------------------------------
   ---------- TCP options ----------
   ---------------------------------
*/
#define LWIP_TCP                        1
#define TCP_MAXRTX                      12
#define TCP_SYNMAXRTX                   6
#define TCP_QUEUE_OOSEQ                 0
#define TCP_MSS                         128
#define TCP_CALCULATE_EFF_SEND_MSS      1

/*
   ----------------------------------
   ---------- Pbuf options ----------
   ----------------------------------
*/
#define PBUF_LINK_HLEN                  (14 + ETH_PAD_SIZE)
#define PBUF_POOL_BUFSIZE               LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_HLEN)

/*
   ------------------------------------------------
   ---------- Network Interfaces options ----------
   ------------------------------------------------
*/
#define LWIP_NETIF_HOSTNAME             0
#define LWIP_NETIF_API                  0
#define LWIP_NETIF_STATUS_CALLBACK      0
#define LWIP_NETIF_LINK_CALLBACK        0
#define LWIP_NETIF_HWADDRHINT           0
#define LWIP_NETIF_LOOPBACK             0

/*
   ----------------------------------------------
   ---------- Sequential layer options ----------
   ----------------------------------------------
*/
#define LWIP_TCPIP_CORE_LOCKING         0
#define LWIP_TCPIP_CORE_LOCKING_INPUT   0
#define LWIP_NETCONN                    0

/*
   ----------------------------------------
   ---------- Statistics options ----------
   ----------------------------------------
*/
#define LWIP_STATS                      1
#define LWIP_STATS_DISPLAY              1

/*
   ---------------------------------
   ---------- PPP options ----------
   ---------------------------------
*/
#define PPP_SUPPORT                     0

/*
   --------------------------------------
   ---------- Checksum options ----------
   --------------------------------------
*/
/* FIXME */
#define CHECKSUM_GEN_IP                 0
#define CHECKSUM_GEN_UDP                0
#define CHECKSUM_GEN_TCP                0
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define LWIP_CHECKSUM_ON_COPY           0

/*
   ---------------------------------------
   ---------- Debugging options ----------
   ---------------------------------------
*/
#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_ALL
#define LWIP_DBG_TYPES_ON               LWIP_DBG_ON
#define ETHARP_DEBUG                    LWIP_DBG_OFF
#define NETIF_DEBUG                     LWIP_DBG_OFF
#define PBUF_DEBUG                      LWIP_DBG_OFF
#define API_LIB_DEBUG                   LWIP_DBG_OFF
#define API_MSG_DEBUG                   LWIP_DBG_OFF
#define SOCKETS_DEBUG                   LWIP_DBG_OFF
#define ICMP_DEBUG                      LWIP_DBG_OFF
#define IGMP_DEBUG                      LWIP_DBG_OFF
#define INET_DEBUG                      LWIP_DBG_OFF
#define IP_DEBUG                        LWIP_DBG_OFF
#define IP_REASS_DEBUG                  LWIP_DBG_OFF
#define RAW_DEBUG                       LWIP_DBG_OFF
#define MEM_DEBUG                       LWIP_DBG_OFF
#define MEMP_DEBUG                      LWIP_DBG_OFF
#define SYS_DEBUG                       LWIP_DBG_OFF
#define TIMERS_DEBUG                    LWIP_DBG_OFF
#define TCP_DEBUG                       LWIP_DBG_OFF
#define TCP_INPUT_DEBUG                 LWIP_DBG_OFF
#define TCP_FR_DEBUG                    LWIP_DBG_OFF
#define TCP_RTO_DEBUG                   LWIP_DBG_OFF
#define TCP_CWND_DEBUG                  LWIP_DBG_OFF
#define TCP_WND_DEBUG                   LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG                LWIP_DBG_OFF
#define TCP_RST_DEBUG                   LWIP_DBG_OFF
#define TCP_QLEN_DEBUG                  LWIP_DBG_OFF
#define UDP_DEBUG                       LWIP_DBG_OFF
#define TCPIP_DEBUG                     LWIP_DBG_OFF
#define PPP_DEBUG                       LWIP_DBG_OFF
#define SLIP_DEBUG                      LWIP_DBG_OFF
#define DHCP_DEBUG                      LWIP_DBG_OFF
#define AUTOIP_DEBUG                    LWIP_DBG_OFF
#define SNMP_MSG_DEBUG                  LWIP_DBG_OFF
#define SNMP_MIB_DEBUG                  LWIP_DBG_OFF
#define DNS_DEBUG                       LWIP_DBG_OFF
#define IP6_DEBUG                       LWIP_DBG_OFF

/* disabled stuff */
#define LWIP_RAW                        0
#define LWIP_AUTOIP                     0
#define LWIP_SNMP                       0
#define LWIP_IGMP                       0
#define LWIP_DNS                        0
#define LWIP_HAVE_LOOPIF                0
#define LWIP_HAVE_SLIPIF                0
#define LWIP_SOCKET                     0
#define LWIP_IPV6                       0


#endif /* __LWIPOPT_H__ */
