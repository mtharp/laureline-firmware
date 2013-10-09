/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "common.h"
#include "cmdline.h"
#include "stm32/eth_mac.h"
#include "mii.h"
#include "init.h"
#include "lwip/def.h"
#include "eeprom.h"
#include "tcpip.h"
#include "uptime.h"
#include "profile.h"
#include "util/parse.h"
//#include "version.h"
#include "cmdline/cmdline.h"
#include "cmdline/settings.h"
#define VERSION "1.2.3.4"


static void cliDefaults(char *cmdline);
static void cliInfo(char *cmdline);
static void cliSave(char *cmdline);
static void cliUptime(char *cmdline);
static void cliVersion(char *cmdline);

static void cli_print_hwaddr(void);
static void cli_print_netif(void);

/* Keep sorted */
const clicmd_t cmd_table[] = {
	{ "defaults", "reset to factory defaults and reboot", cliDefaults },
	{ "exit", "leave command mode", cli_cmd_exit },
	{ "help", "", cli_cmd_help },
	{ "info", "show runtime information", cliInfo },
#ifdef PROFILE_TASKS
	{ "profile", "", cli_cmd_profile },
#endif
	{ "save", "save changes and reboot", cliSave },
	{ "set", "name=value or blank or * for list", cli_cmd_set },
	{ "uptime", "show the system uptime", cliUptime },
	{ "version", "show version", cliVersion },
	{ NULL },
};


const clivalue_t value_table[] = {
	{ "admin_key", VAR_HEX, &cfg.admin_key, 8 },
	{ "gps_baud_rate", VAR_UINT32, &cfg.gps_baud_rate, 0 },
	{ "ip_addr", VAR_IP4, &cfg.ip_addr, 0 },
	{ "ip_gateway", VAR_IP4, &cfg.ip_gateway, 0 },
	{ "ip_netmask", VAR_IP4, &cfg.ip_netmask, 0 },
	{ NULL },
};



/* Command implementation */

static void
cliWriteConfig(void) {
	int16_t result;
	cli_puts("Writing EEPROM...\r\n");
	result = eeprom_write_cfg();
	if (result == EERR_TIMEOUT) {
		cli_puts("ERROR: timeout while writing EEPROM\r\n");
	} else if (result == EERR_NACK) {
		cli_puts("ERROR: EEPROM is faulty or missing\r\n");
	} else if (result == EERR_FAULT) {
		cli_puts("ERROR: EEPROM is faulty\r\n");
	} else if (result != EERR_OK) {
		cli_puts("FAIL: unable to write EEPROM\r\n");
	} else {
		cli_puts("OK\r\n");
		CoTickDelay(S2ST(1));
		NVIC_SystemReset();
	}
}


static void
cliDefaults(char *cmdline) {
	memset(cfg_bytes, 0, EEPROM_CFG_SIZE);
	cfg.version = CFG_VERSION;
	cliWriteConfig();
}


static void
cliInfo(char *cmdline) {
	cliVersion(NULL);
	cli_print_hwaddr();
	cli_print_link();
	cli_print_netif();
	cliUptime(NULL);
	cli_printf("System clock:   %d Hz (nominal)\r\n", (int)system_frequency);
}


static void
cliSave(char *cmdline) {
	cliWriteConfig();
}


static void
cliUptime(char *cmdline) {
	cli_puts("Uptime:         ");
	cli_puts(uptime_format());
	cli_puts("\r\n");
}


static void
cliVersion(char *cmdline) {
	cli_puts(
		"Hardware:       " BOARD_REV "\r\n"
		"Software:       " VERSION "\r\n");
}


static void
cli_print_hwaddr(void) {
	cli_printf("MAC Address:    %02x:%02x:%02x:%02x:%02x:%02x\r\n",
			thisif.hwaddr[0], thisif.hwaddr[1], thisif.hwaddr[2],
			thisif.hwaddr[3], thisif.hwaddr[4], thisif.hwaddr[5]);
}


static void
print_ipaddr(uint32_t addr) {
	cli_printf("%d.%d.%d.%d",
			(addr      ) & 0xff,
			(addr >>  8) & 0xff,
			(addr >> 16) & 0xff,
			(addr >> 24) & 0xff);
}


static void
cli_print_netif(void) {
	cli_puts("IP:             ");
	print_ipaddr(thisif.ip_addr.addr);
	cli_puts("\r\nNetmask:        ");
	print_ipaddr(thisif.netmask.addr);
	cli_puts("\r\nGateway:        ");
	print_ipaddr(thisif.gw.addr);
	cli_puts("\r\n");
}


void
cli_print_link(void) {
	uint32_t bmcr, bmsr, lpa;
	bmsr = smi_read(MII_BMSR);
	bmcr = smi_read(MII_BMCR);
	lpa = smi_read(MII_LPA);
	cli_puts("Link status:    ");
	if (bmcr & BMCR_ANENABLE) {
		if ((bmsr & (BMSR_LSTATUS | BMSR_RFAULT | BMSR_ANEGCOMPLETE))
				!= (BMSR_LSTATUS | BMSR_ANEGCOMPLETE)) {
			cli_puts("Down\r\n");
		} else {
			cli_puts("Auto ");
			if (lpa & (LPA_100HALF | LPA_100FULL | LPA_100BASE4)) {
				cli_puts("100M ");
			} else {
				cli_puts("10M ");
			}
			if (lpa & (LPA_10FULL | LPA_100FULL)) {
				cli_puts("Full\r\n");
			} else {
				cli_puts("Half\r\n");
			}
		}
	} else {
		if (!(bmsr & BMSR_LSTATUS)) {
			cli_puts("Down\r\n");
		} else {
			cli_puts("Manual ");
			if (bmcr & BMCR_SPEED100) {
				cli_puts("100M ");
			} else {
				cli_puts("10M ");
			}
			if (bmcr & BMCR_FULLDPLX) {
				cli_puts("Full\r\n");
			} else {
				cli_puts("Half\r\n");
			}
		}
	}
}


void
cli_banner(void) {
	cli_puts("\r\n\r\nLaureline GPS NTP Server\r\n");
	cliVersion(NULL);
	cli_print_hwaddr();
	cli_puts("\r\nPress Enter to enable command-line\r\n");
}
