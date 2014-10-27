/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "semphr.h"
#include "task.h"

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
#include "net/tcpqueue.h"

#include "lwip/dhcp.h"
#include "lwip/ethip6.h"
#include "lwip/igmp.h"
#include "lwip/init.h"
#include "lwip/mld6.h"
#include "lwip/snmp.h"
#include "lwip/stats.h"
#include "lwip/timers.h"
#include "netif/etharp.h"

#include <stdio.h>

TaskHandle_t thread_tcpip;

struct netif thisif;
QueueHandle_t tcpip_queue;

static int did_startup;

static void tcpip_thread(void *p);
static void link_changed(void);
static void interface_changed(struct netif *netif);
static void configure_interface(void);
static err_t ethernetif_init(struct netif *netif);
static err_t low_level_output(struct netif *netif, struct pbuf *p);
static int ethernetif_input(struct netif *netif);

static const uint8_t sysdescr_len = sizeof(BOARD_NAME) - 1;

/* milliseconds */
#define TCPIP_CHECKS_TMR_INTERVAL 1000


void
tcpip_start(void) {
    ASSERT((tcpip_queue = xQueueCreate(8, sizeof(void*))));
    lwip_init();
    snmp_set_sysdescr((const u8_t*)BOARD_NAME, &sysdescr_len);
    api_start();
    if (cfg.gps_listen_port) {
        relay_server_start(cfg.gps_listen_port);
    }
    if (cfg.syslog_ip) {
        syslog_start(cfg.syslog_ip);
    }
    configure_interface();
    ntp_server_start();

    ASSERT(xTaskCreate(tcpip_thread, "tcpip", TCPIP_STACK_SIZE, NULL,
                THREAD_PRIO_TCPIP, &thread_tcpip));
}


static void
tcpip_checks(void *arg) {
#if LWIP_IPV6
    static int valid_ip6 = 0;
    int i;
#endif
    watchdog_net = 5;
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
                        "IPv6 address assigned: " IP6_DIGITS_FMT "%s",
                        IP6_ADDR_BLOCK1(addr),
                        IP6_ADDR_BLOCK2(addr),
                        IP6_ADDR_BLOCK3(addr),
                        IP6_ADDR_BLOCK4(addr),
                        IP6_ADDR_BLOCK5(addr),
                        IP6_ADDR_BLOCK6(addr),
                        IP6_ADDR_BLOCK7(addr),
                        IP6_ADDR_BLOCK8(addr),
                        ip6_addr_islinklocal(addr) ? " (link-local)" : "");
                if (i == 0 && !ip6_addr_isany(&cfg.ip6_manycast)) {
                    /* Link-local address added, now we can join multicast groups */
                    mld6_joingroup(netif_ip6_addr(&thisif, 0), &cfg.ip6_manycast);
                }
            }
        } else if ((valid_ip6 >> i) & 1) {
            /* address removed */
            valid_ip6 &= ~(1 << i);
        }
    }
#endif
    sys_timeout(TCPIP_CHECKS_TMR_INTERVAL, tcpip_checks, NULL);
}


static void
tcpip_thread(void *p) {
    void *msg;
    int frame_received = 0;
    api_set_main_thread(xTaskGetCurrentTaskHandle());
    if (cfg.ip_addr.addr == 0 || cfg.ip_netmask.addr == 0) {
        dhcp_start(&thisif);
    } else {
        netif_set_up(&thisif);
    }
    sys_timeout(TCPIP_CHECKS_TMR_INTERVAL, tcpip_checks, NULL);
    while (1) {
        /* If a frame was received last cycle then check for another one
         * immediately, but still peek in the queue first. If no frame was
         * received then sleep between events.
         */
        if (xQueueReceive(tcpip_queue, &msg,
                    frame_received ? 0 : pdMS_TO_TICKS(20))) {
            if (msg != NULL) {
                /* API call */
                api_accept(msg);
            }
        }
        sys_check_timeouts();
        frame_received = ethernetif_input(&thisif);
    }
}


static void
link_changed(void) {
    char buf[SMI_DESCRIBE_SIZE];
    smi_describe_link(buf);
    log_write(LOG_NOTICE, "net", "Link changed status: %s", buf);
    if (!thisif.dhcp && !did_startup) {
        did_startup = 1;
        log_startup();
    }
}


static void
interface_changed(struct netif *netif) {
    if (cfg.ip_manycast.addr != 0) {
        if (thisif.flags & NETIF_FLAG_UP) {
            igmp_joingroup(IP_ADDR_ANY, &cfg.ip_manycast);
        } else {
            igmp_leavegroup(IP_ADDR_ANY, &cfg.ip_manycast);
        }
    }
    if (!(thisif.flags & NETIF_FLAG_UP)) {
        return;
    }
    {
        char buf[16];
        sprintf(buf, IP_DIGITS_FMT, IP_DIGITS(&thisif.ip_addr));
        log_sethostname(buf);
    }
    if (thisif.dhcp) {
        if (!did_startup) {
            did_startup = 1;
            log_startup();
        }
        log_write(LOG_NOTICE, "net",
                "IP address acquired from DHCP: " IP_DIGITS_FMT,
                IP_DIGITS(&thisif.ip_addr));
    }
}



static void
configure_interface(void) {
    long *seed = (long*)&thisif.hwaddr[2];
    ASSERT(eeprom_read(0xFA, thisif.hwaddr, 6) == EERR_OK);
    mac_start();
    mac_set_hwaddr(thisif.hwaddr);
    SRAND(*seed);

    netif_add(&thisif, &cfg.ip_addr, &cfg.ip_netmask, &cfg.ip_gateway, NULL,
            ethernetif_init, ethernet_input);

    netif_set_default(&thisif);
    netif_set_status_callback(&thisif, interface_changed);
    /* TODO: IGMP filter. Allowing all multicast for now.  */
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
    err_t rc;
    pbuf_header(p, -ETH_PAD_SIZE);
    rc = maczero_transmit(p, MS2ST(50));
    if (rc) {
        snmp_inc_ifoutdiscards(netif);
    } else {
        LINK_STATS_INC(link.xmit);
        snmp_add_ifoutoctets(netif, p->tot_len);
        snmp_inc_ifoutucastpkts(netif);
    }
    return rc;
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
