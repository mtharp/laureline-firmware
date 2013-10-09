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
#include "util/parse.h"
//#include "version.h"
#define VERSION "1.2.3.4"

#ifdef PROFILE_TASKS
#include "OsTask.h"
static void cliProfile(char *cmdline);
#endif


uint8_t cl_enabled;
serial_t *cl_out;

static char cl_buf[64];
static uint8_t cl_count;

static void cliDefaults(char *cmdline);
static void cliExit(char *cmdline);
static void cliHelp(char *cmdline);
static void cliInfo(char *cmdline);
static void cliSave(char *cmdline);
static void cliSet(char *cmdline);
static void cliUptime(char *cmdline);
static void cliVersion(char *cmdline);

static void cli_print_hwaddr(void);
static void cli_print_netif(void);


typedef struct {
	char *name;
	char *param;
	void (*func)(char *cmdline);
} clicmd_t;


typedef enum {
	VAR_UINT32,
	VAR_BOOL,
	VAR_IP4,
	VAR_HEX
} vartype_e;


typedef struct {
	const char *name;
	const uint8_t type; /* vartype_e */
	void *ptr;
	uint8_t len;
} clivalue_t;

static void cliSetVar(const clivalue_t *var, const char *valstr);
static void cliPrintVar(const clivalue_t *var, uint8_t full);

/* Keep sorted */
const clicmd_t cmd_table[] = {
	{ "defaults", "reset to factory defaults and reboot", cliDefaults },
	{ "exit", "leave command mode", cliExit },
	{ "help", "", cliHelp },
	{ "info", "show runtime information", cliInfo },
#ifdef PROFILE_TASKS
	{ "profile", "", cliProfile },
#endif
	{ "save", "save changes and reboot", cliSave },
	{ "set", "name=value or blank or * for list", cliSet },
	{ "uptime", "show the system uptime", cliUptime },
	{ "version", "show version", cliVersion },
};
#define CMD_COUNT (sizeof(cmd_table) / sizeof(cmd_table[0]))

const clivalue_t value_table[] = {
	{ "admin_key", VAR_HEX, &cfg.admin_key, 8 },
	{ "gps_baud_rate", VAR_UINT32, &cfg.gps_baud_rate, 0 },
	{ "ip_addr", VAR_IP4, &cfg.ip_addr, 0 },
	{ "ip_gateway", VAR_IP4, &cfg.ip_gateway, 0 },
	{ "ip_netmask", VAR_IP4, &cfg.ip_netmask, 0 },
};
#define VALUE_COUNT (sizeof(value_table) / sizeof(value_table[0]))


/* Initialization and UART handling */

void
cli_set_output(serial_t *output) {
	cl_out = output;
}


/* Command-line helpers */

static void
cliPrompt(void) {
	cl_count = 0;
	cl_enabled = 1;
	cli_puts("\r\n# ");
}


static int
cliCompare(const void *a, const void *b) {
	const clicmd_t *ca = a, *cb = b;
	return strncasecmp(ca->name, cb->name, strlen(cb->name));
}


static void
cliPrintVar(const clivalue_t *var, uint8_t full) {
	switch (var->type) {
	case VAR_UINT32:
		cli_printf("%u", *(uint32_t*)var->ptr);
		break;
	case VAR_BOOL:
		cli_printf("%u", !!*(uint8_t*)var->ptr);
		break;
	case VAR_IP4:
		{
			uint8_t *addr = (uint8_t*)var->ptr;
			cli_printf("%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
			break;
		}
	case VAR_HEX:
		{
			uint8_t *ptr = (uint8_t*)var->ptr;
			int i;
			for (i = 0; i < var->len; i++) {
				cli_printf("%02x", *ptr++);
			}
		}
	}
}


static void
cliSetVar(const clivalue_t *var, const char *str) {
	uint32_t val = 0;
	uint8_t val2 = 0;
	switch (var->type) {
	case VAR_UINT32:
		*(uint32_t*)var->ptr = atoi_decimal(str);
		break;
	case VAR_BOOL:
		*(uint8_t*)var->ptr = !!atoi_decimal(str);
		break;
	case VAR_IP4:
		while (*str) {
			if (*str == '.') {
				val = (val << 8) | val2;
				val2 = 0;
			} else if (*str >= '0' && *str <= '9') {
				val2 = val2 * 10 + (*str - '0');
			}
			str++;
		}
		val = (val << 8) | val2;
		*(uint32_t*)var->ptr = lwip_htonl(val);
		break;
	case VAR_HEX:
		{
			uint8_t *ptr = (uint8_t*)var->ptr;
			int i;
			for (i = 0; i < var->len; i++) {
				val2 = *str++;
				if (val2 == 0) {
					break;
				} else {
					val = parse_hex(val2) << 4;
				}
				val2 = *str++;
				if (val2 == 0) {
					break;
				} else {
					val |= parse_hex(val2);
				}
				*ptr++ = val;
			}
			/* Pad the remainder with zeroes */
			for (; i < var->len; i++) {
				*ptr++ = 0;
			}
		}
	}
}


void
cli_feed(char c) {
	if (cl_enabled == 0 && c != '\r' && c != '\n') {
		return;
	}
	cl_enabled = 1;

	if (c == '\t' || c == '?') {
		/* Tab completion */
		/* TODO */
	} else if (!cl_count && c == 4) {
		/* EOF */
		cliExit(cl_buf);
	} else if (c == 12) {
		/* Clear screen */
		cli_puts("\033[2J\033[1;1H");
		cliPrompt();
	} else if (c == '\n' || c == '\r') {
		if (cl_count) {
			/* Enter pressed */
			clicmd_t *cmd = NULL;
			clicmd_t target;
			cli_puts("\r\n");
			cl_buf[cl_count] = 0;

			target.name = cl_buf;
			target.param = NULL;
			cmd = bsearch(&target, cmd_table, CMD_COUNT, sizeof(cmd_table[0]),
					cliCompare);
			if (cmd) {
				cmd->func(cl_buf + strlen(cmd->name) + 1);
			} else {
				cli_puts("ERR: Unknown command, try 'help'\r\n");
			}
			memset(cl_buf, 0, sizeof(cl_buf));
		} else if (c == '\n' && cl_buf[0] == '\r') {
			/* Ignore \n after \r */
			return;
		}
		if (cl_enabled) {
			cliPrompt();
		}
		cl_buf[0] = c;
	} else if (c == 8 || c == 127) {
		/* Backspace */
		if (cl_count) {
			cl_buf[--cl_count] = 0;
			cli_puts("\010 \010");
		}
	} else if (cl_count < sizeof(cl_buf) && c >= 32 && c <= 126) {
		if (!cl_count && c == 32) {
			return;
		}
		cl_buf[cl_count++] = c;
		serial_write(cl_out, &c, 1);
	}
}


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
cliExit(char *cmdline) {
	cl_count = 0;
	cl_enabled = 0;
	cli_puts("Exiting cmdline mode.\r\n"
			"Configuration changes have not been saved.\r\n"
			"Press Enter to enable cmdline.\r\n");
}


static void
cliHelp(char *cmdline) {
	uint8_t i;
	cli_puts("Available commands:\r\n");
	for (i = 0; i < CMD_COUNT; i++) {
		cli_printf("%s\t%s\r\n", cmd_table[i].name, cmd_table[i].param);
	}
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


#ifdef PROFILE_TASKS
#define NUM_IRQS 72
extern U64 isr_ticks[NUM_IRQS];

static void
cliProfile(char *cmdline) {
	uint64_t task_counts[CFG_MAX_USER_TASKS+SYS_TASK_NUM];
	//uint64_t irq_counts[NUM_IRQS];
	uint64_t count, pct, total = 0;
	int i;

	DISABLE_IRQ();
	for(i = 0; i < CFG_MAX_USER_TASKS+SYS_TASK_NUM; i++) {
		task_counts[i] = count = TCBTbl[i].tick_count;
		total += count;
	}
	/*
	for(i = 0; i < NUM_IRQS; i++) {
		irq_counts[i] = count = isr_ticks[i];
		total += count;
	}*/
	ENABLE_IRQ();

	cli_printf("#      Counts         %%    Name\r\n");
	for(i = 0; i < CFG_MAX_USER_TASKS+SYS_TASK_NUM; i++) {
		count = task_counts[i];
		pct = 10000 * count / total;
		cli_printf("%2d %08x%08x %3u.%02u%% %s\r\n",
				i,
				(uint32_t)(count >> 32),
				(uint32_t)count,
				(uint32_t)(pct / 100),
				(uint32_t)(pct % 100),
				TCBTbl[i].name);
	}
	/*
	cli_printf(" -- interrupt handlers --\r\n");
	for(i = 0; i < NUM_IRQS; i++) {
		count = irq_counts[i];
		if (count == 0) {
			continue;
		}
		cli_printf("%2d %08x%08x %3u.%02u%% %s\r\n",
				i,
				(uint32_t)(count >> 32),
				(uint32_t)count,
				(uint32_t)(pct / 100),
				(uint32_t)(pct % 100),
				"");
	}*/
}
#endif


static void
cliSave(char *cmdline) {
	cliWriteConfig();
}


static void
cliSet(char *cmdline) {
	uint32_t i, len;
	const clivalue_t *val;
	char *eqptr = NULL;

	len = strlen(cmdline);
	if (len == 0 || (len == 1 && cmdline[0] == '*')) {
		cli_puts("Current settings:\r\n");
		for (i = 0; i < VALUE_COUNT; i++) {
			val = &value_table[i];
			cli_printf("%s = ", value_table[i].name);
			cliPrintVar(val, len);
			cli_puts("\r\n");
		}
	} else if ((eqptr = strstr(cmdline, "="))) {
		eqptr++;
		len--;
		while (*eqptr == ' ') {
			eqptr++;
			len--;
		}
		for (i = 0; i < VALUE_COUNT; i++) {
			val = &value_table[i];
			if (strncasecmp(cmdline, value_table[i].name,
						strlen(value_table[i].name)) == 0) {
				cliSetVar(val, eqptr);
				cli_printf("%s set to ", value_table[i].name);
				cliPrintVar(val, 0);
				return;
			}
		}
		cli_puts("ERR: Unknown variable name\r\n");
	}
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
