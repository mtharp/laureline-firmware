/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "eeprom.h"
#include "eth_mac.h"
#include "tcpip.h"

#define TCPIP_STACK 512
OS_STK tcpip_stack[TCPIP_STACK];
OS_TID tcpip_tid;

OS_FlagID rx_flag, tx_flag;

uint8_t tcpip_hwaddr[6];

static void tcpip_thread(void *p);


void
tcpip_start(void) {
	tcpip_tid = CoCreateTask(tcpip_thread, NULL, THREAD_PRIO_TCPIP,
			&tcpip_stack[TCPIP_STACK-1], TCPIP_STACK);
	ASSERT(tcpip_tid != E_CREATE_FAIL);
	ASSERT((rx_flag = CoCreateFlag(1, 0)) != E_CREATE_FAIL);
	ASSERT((tx_flag = CoCreateFlag(1, 0)) != E_CREATE_FAIL);
}


static void
configure_interfaces(void) {
	//struct ip_addr ip, gateway, netmask;
	ASSERT(eeprom_read(0xFA, tcpip_hwaddr, 6) == E_OK);
	mac_start();
	mac_set_hwaddr(tcpip_hwaddr);
	/*ip.addr = cfg.ip_addr;
	gateway.addr = cfg.ip_gateway;
	netmask.addr = cfg.ip_netmask;
#if NO_SYS
	netif_add(&thisif, &ip, &netmask, &gateway, NULL, ethernetif_init, ethernet_input);
#else
	netif_add(&thisif, &ip, &netmask, &gateway, NULL, ethernetif_init, tcpip_input);
#endif

	netif_set_default(&thisif);
	netif_set_up(&thisif);
	if (ip.addr == 0 || netmask.addr == 0) {
		dhcp_start(&thisif);
	}*/
}


static void
tcpip_thread(void *p) {
	configure_interfaces();
	while (1) {
		if (smi_poll_link_status()) {
			GPIO_OFF(ETH_LED);
		} else {
			GPIO_ON(ETH_LED);
		}
		CoTickDelay(S2ST(1));
	}
}
