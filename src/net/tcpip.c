/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "timers.h"

#include "cmdline.h"
#include "eeprom.h"
#include "logging.h"
#include "main.h"
#include "stm32/eth_mac.h"
#include "net/ntpclient.h"
#include "net/ntpserver.h"
#include "net/relay.h"
#include "net/tcpapi.h"
#include "net/tcpip.h"

#include "lwip/dhcp.h"
#include "lwip/ethip6.h"
#include "lwip/igmp.h"
#include "lwip/init.h"
#include "lwip/mld6.h"
#include "lwip/snmp.h"
#include "lwip/stats.h"
#include "lwip/timers.h"
#include "netif/etharp.h"

TaskHandle_t thread_tcpip;

struct netif thisif;
static int did_startup;

static void tcpip_thread(void *p);
static void link_changed(void);
static void interface_changed(struct netif *netif);
static void configure_interface(void);
static err_t ethernetif_init(struct netif *netif);
static err_t low_level_output(struct netif *netif, struct pbuf *p);
static int ethernetif_input(struct netif *netif);
static void tcpip_timer(TimerHandle_t timer);

static const uint8_t sysdescr_len = sizeof(BOARD_NAME) - 1;


void
tcpip_start(void) {
    TimerHandle_t timer;
    lwip_init();
    snmp_set_sysdescr((const u8_t*)BOARD_NAME, &sysdescr_len);
    api_start();
    configure_interface();
    ntp_server_start();
    if (cfg.gps_listen_port) {
        relay_server_start(cfg.gps_listen_port);
    }
    if (cfg.syslog_ip) {
        syslog_start(cfg.syslog_ip);
    }

    ASSERT((timer = xTimerCreate("tcpip", pdMS_TO_TICKS(1000), 1, NULL, tcpip_timer)));
    xTimerStart(timer, 0);

    ASSERT(xTaskCreate(tcpip_thread, "tcpip", TCPIP_STACK_SIZE, NULL,
                THREAD_PRIO_TCPIP, &thread_tcpip));
}


static void
tcpip_timer(TimerHandle_t timer) {
    xEventGroupSetBits(mac_events, TIMER_FLAG);
}


static void
tcpip_thread(void *p) {
    EventBits_t flags;
#if LWIP_IPV6
    int i, valid_ip6 = 0;
#endif
    api_set_main_thread(xTaskGetCurrentTaskHandle());
    while (1) {
        flags = xEventGroupWaitBits(mac_events,
                MAC_RX_FLAG | API_FLAG | TIMER_FLAG,
                1, 0, portMAX_DELAY);
        if (flags & TIMER_FLAG) {
            if (smi_poll_link_status()) {
                if (!netif_is_link_up(&thisif)) {
                    link_changed();
                    netif_set_link_up(&thisif);
                }
                GPIO_OFF(ETH_LED);
            } else {
                if (netif_is_link_up(&thisif)) {
                    link_changed();
                    netif_set_link_down(&thisif);
                }
                GPIO_ON(ETH_LED);
            }
#if LWIP_IPV6
            for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
                if (ip6_addr_isvalid(netif_ip6_addr_state(&thisif, i))) {
                    if (!((valid_ip6 >> i) & 1)) {
                        ip6_addr_t *addr = netif_ip6_addr(&thisif, i);
                        valid_ip6 |= (1 << i);
                        log_write(LOG_NOTICE, "net",
                                "IPv6 address assigned: %x:%x:%x:%x:%x:%x:%x:%x%s",
                                IP6_ADDR_BLOCK1(addr),
                                IP6_ADDR_BLOCK2(addr),
                                IP6_ADDR_BLOCK3(addr),
                                IP6_ADDR_BLOCK4(addr),
                                IP6_ADDR_BLOCK5(addr),
                                IP6_ADDR_BLOCK6(addr),
                                IP6_ADDR_BLOCK7(addr),
                                IP6_ADDR_BLOCK8(addr),
                                ip6_addr_islinklocal(addr) ? " (link-local)" : "");
                        if (i == 0 && cfg.ip6_manycast[0] != 0) {
                            /* Link-local address added, now we can join multicast groups */
                            ip6_addr_t group;
                            group.addr[0] = cfg.ip6_manycast[0];
                            group.addr[1] = cfg.ip6_manycast[1];
                            group.addr[2] = cfg.ip6_manycast[2];
                            group.addr[3] = cfg.ip6_manycast[3];
                            mld6_joingroup(netif_ip6_addr(&thisif, 0), &group);
                        }
                    }
                } else if ((valid_ip6 >> i) & 1) {
                    /* address removed */
                    valid_ip6 &= ~(1 << i);
                }
            }
#endif
            sys_check_timeouts();
        }
        if (flags & MAC_RX_FLAG) {
            while (ethernetif_input(&thisif)) {}
        }
        if (flags & API_FLAG) {
            api_accept();
        }
    }
}


static void
link_changed(void) {
    char buf[SMI_DESCRIBE_SIZE];
    smi_describe_link(buf);
    log_write(LOG_NOTICE, "net", "Link changed status: %s", buf);
}


static void
interface_changed(struct netif *netif) {
    if (cfg.ip_manycast) {
        ip_addr_t ip;
        ip.addr = cfg.ip_manycast;
        if (thisif.flags & NETIF_FLAG_UP) {
            igmp_joingroup(IP_ADDR_ANY, &ip);
        } else {
            igmp_leavegroup(IP_ADDR_ANY, &ip);
        }
    }
    if (!(thisif.flags & NETIF_FLAG_UP)) {
        return;
    }
    if (!did_startup) {
        did_startup = 1;
        log_startup();
    }
    if (thisif.dhcp) {
        log_write(LOG_NOTICE, "net",
                "IP address acquired from DHCP: %d.%d.%d.%d",
                IP_DIGITS(thisif.ip_addr.addr));
    }
}



static void
configure_interface(void) {
    struct ip_addr ip, gateway, netmask;
    long *seed = (long*)&thisif.hwaddr[2];
    ASSERT(eeprom_read(0xFA, thisif.hwaddr, 6) == EERR_OK);
    mac_start();
    mac_set_hwaddr(thisif.hwaddr);
    SRAND(*seed);

    ip.addr = cfg.ip_addr;
    gateway.addr = cfg.ip_gateway;
    netmask.addr = cfg.ip_netmask;
    netif_add(&thisif, &ip, &netmask, &gateway, NULL, ethernetif_init, ethernet_input);

    netif_set_default(&thisif);
    netif_set_status_callback(&thisif, interface_changed);
    /* TODO: IGMP filter. Allowing all multicast for now.  */
    if (ip.addr == 0 || netmask.addr == 0) {
        dhcp_start(&thisif);
    } else {
        netif_set_up(&thisif);
    }
}


static err_t
ethernetif_init(struct netif *netif) {
    netif->state = NULL;
    netif->name[0] = 'e';
    netif->name[1] = 'n';
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
    netif->ip6_autoconfig_enabled = 1;
    netif_create_ip6_linklocal_address(netif, 1);
#endif
    return ERR_OK;
}


static err_t
low_level_output(struct netif *netif, struct pbuf *p) {
    struct pbuf *q;
    mac_desc_t *tdes;
    tdes = mac_get_tx_descriptor(pdMS_TO_TICKS(50));
    if (tdes == NULL) {
        LINK_STATS_INC(link.err);
        LINK_STATS_INC(link.drop);
        snmp_inc_ifoutdiscards(netif);
        return ERR_TIMEOUT;
    }
    pbuf_header(p, -ETH_PAD_SIZE);
    for (q = p; q != NULL; q = q->next) {
        mac_write_tx_descriptor(tdes, q->payload, q->len);
    }
    mac_release_tx_descriptor(tdes);
    pbuf_header(p, ETH_PAD_SIZE);
    LINK_STATS_INC(link.xmit);
    snmp_add_ifoutoctets(netif, p->tot_len);
    snmp_inc_ifoutucastpkts(netif);
    return ERR_OK;
}


static int
ethernetif_input(struct netif *netif) {
    struct pbuf *p, *q;
    uint16_t len;
    mac_desc_t *rdesc;

    (void)netif;
    if ((rdesc = mac_get_rx_descriptor()) == NULL) {
        return 0;
    }
    len = rdesc->size + ETH_PAD_SIZE;
    if ((p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL)) == NULL) {
        mac_release_rx_descriptor(rdesc);
        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
        snmp_inc_ifindiscards(netif);
        return 1;
    }
    pbuf_header(p, -ETH_PAD_SIZE);
    for (q = p; q != NULL; q = q->next) {
        mac_read_rx_descriptor(rdesc, q->payload, q->len);
    }
    mac_release_rx_descriptor(rdesc);
    pbuf_header(p, ETH_PAD_SIZE);
    LINK_STATS_INC(link.recv);
    snmp_inc_ifinucastpkts(netif);
    snmp_add_ifinoctets(netif, p->tot_len);
    if (netif->input(p, netif) != ERR_OK) {
        pbuf_free(p);
    }
    return 1;
}
