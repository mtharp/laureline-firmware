/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "cmdline.h"
#include "eeprom.h"
#include "logging.h"
#include "main.h"
#include "stm32/eth_mac.h"
#include "ntpserver.h"
#include "relay.h"
#include "tcpapi.h"
#include "tcpip.h"

#include "lwip/dhcp.h"
#include "lwip/init.h"
#include "lwip/stats.h"
#include "lwip/timers.h"
#include "netif/etharp.h"

#define TCPIP_STACK 512
OS_STK tcpip_stack[TCPIP_STACK];
OS_TID tcpip_tid;

OS_TCID timer;
OS_FlagID timer_flag;

struct netif thisif;

static void tcpip_thread(void *p);
static void link_changed(void);
static void interface_changed(struct netif *netif);
static void configure_interface(void);
static err_t ethernetif_init(struct netif *netif);
static err_t low_level_output(struct netif *netif, struct pbuf *p);
static int ethernetif_input(struct netif *netif);
static void tcpip_timer(void);


void
tcpip_start(void) {
	lwip_init();
	api_start();
	configure_interface();
	ntp_server_start();
	if (cfg.gps_listen_port) {
		relay_server_start(cfg.gps_listen_port);
	}
	if (cfg.syslog_ip) {
		syslog_start(cfg.syslog_ip);
	}

	ASSERT((timer_flag = CoCreateFlag(1, 0)) != E_CREATE_FAIL);
	timer = CoCreateTmr(TMR_TYPE_PERIODIC, S2ST(1), S2ST(1), tcpip_timer);
	ASSERT(timer != E_CREATE_FAIL);
	CoStartTmr(timer);

	tcpip_tid = CoCreateTask(tcpip_thread, NULL, THREAD_PRIO_TCPIP,
			&tcpip_stack[TCPIP_STACK-1], TCPIP_STACK, "tcpip");
	ASSERT(tcpip_tid != E_CREATE_FAIL);
}


static void
tcpip_timer(void) {
	isr_SetFlag(timer_flag);
}


static void
tcpip_thread(void *p) {
	uint32_t flags;
	StatusType rc;
	api_set_main_thread(CoGetCurTaskID());
	while (1) {
		flags = CoWaitForMultipleFlags(0
				| (1 << timer_flag)
				| (1 << mac_rx_flag)
				| (1 << api_flag)
				, OPT_WAIT_ANY, 0, &rc);
		ASSERT(rc == E_OK);
		if (flags & (1 << timer_flag)) {
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
			sys_check_timeouts();
		}
		if (flags & (1 << mac_rx_flag)) {
			while (ethernetif_input(&thisif)) {}
		}
		if (flags & (1 << api_flag)) {
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
	if (netif != &thisif || !(thisif.flags & NETIF_FLAG_UP)) {
		return;
	}
	log_startup();
	if (thisif.dhcp) {
		log_write(LOG_NOTICE, "net",
				"IP address acquired from DHCP: %d.%d.%d.%d",
				IP_DIGITS(thisif.ip_addr.addr));
	}
}



static void
configure_interface(void) {
	struct ip_addr ip, gateway, netmask;
	ASSERT(eeprom_read(0xFA, thisif.hwaddr, 6) == E_OK);
	mac_start();
	mac_set_hwaddr(thisif.hwaddr);

	ip.addr = cfg.ip_addr;
	gateway.addr = cfg.ip_gateway;
	netmask.addr = cfg.ip_netmask;
	netif_add(&thisif, &ip, &netmask, &gateway, NULL, ethernetif_init, ethernet_input);

	netif_set_default(&thisif);
	netif_set_status_callback(&thisif, interface_changed);
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
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
	return ERR_OK;
}


static err_t
low_level_output(struct netif *netif, struct pbuf *p) {
	struct pbuf *q;
	mac_desc_t *tdes;
	tdes = mac_get_tx_descriptor(MS2ST(50));
	if (tdes == NULL) {
		return ERR_TIMEOUT;
	}
	pbuf_header(p, -ETH_PAD_SIZE);
	for (q = p; q != NULL; q = q->next) {
		mac_write_tx_descriptor(tdes, q->payload, q->len);
	}
	mac_release_tx_descriptor(tdes);
	pbuf_header(p, ETH_PAD_SIZE);
	LINK_STATS_INC(link.xmit);
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
		return 1;
	}
	pbuf_header(p, -ETH_PAD_SIZE);
	for (q = p; q != NULL; q = q->next) {
		mac_read_rx_descriptor(rdesc, q->payload, q->len);
	}
	mac_release_rx_descriptor(rdesc);
	pbuf_header(p, ETH_PAD_SIZE);
	LINK_STATS_INC(link.recv);
	if (netif->input(p, netif) != ERR_OK) {
		pbuf_free(p);
	}
	return 1;
}
