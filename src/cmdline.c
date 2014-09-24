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
#include "task.h"

#include "cmdline.h"
#include "stm32/eth_mac.h"
#include "mii.h"
#include "info_table.h"
#include "init.h"
#include "lwip/def.h"
#include "eeprom.h"
#include "net/tcpip.h"
#include "uptime.h"
#include "version.h"
#include "util/parse.h"
#include "cmdline/cmdline.h"
#include "cmdline/settings.h"


static void cliDefaults(char *cmdline);
static void cliInfo(char *cmdline);
static void cliSave(char *cmdline);
static void cliUptime(char *cmdline);
static void cliVersion(char *cmdline);
static void cli_cmd_fsnum(char *cmdline);

static void cli_print_hwaddr(void);
static void cli_print_netif(void);
static void cli_print_serial(void);

/* Keep sorted */
const clicmd_t cmd_table[] = {
    { "defaults", "reset to factory defaults and reboot", cliDefaults },
    { "exit", "leave command mode", cli_cmd_exit },
    { "fsnum", NULL, cli_cmd_fsnum },
    { "help", "", cli_cmd_help },
    { "info", "show runtime information", cliInfo },
    { "save", "save changes and reboot", cliSave },
    { "set", "name=value or blank or * for list", cli_cmd_set },
    { "uptime", "show the system uptime", cliUptime },
    { "version", "show version", cliVersion },
    { NULL },
};


const clivalue_t value_table[] = {
    { "admin_key", VAR_HEX, &cfg.admin_key, 8 },
    { "gps_baud_rate", VAR_UINT32, &cfg.gps_baud_rate, 0 },
    { "gps_ext_in", VAR_FLAG, &cfg.flags, FLAG_GPSEXT },
    { "gps_ext_out", VAR_FLAG, &cfg.flags, FLAG_GPSOUT },
    { "gps_listen_port", VAR_UINT16, &cfg.gps_listen_port, 0 },
    { "holdover_test", VAR_FLAG, &cfg.flags, FLAG_HOLDOVER_TEST },
    { "holdover_time", VAR_UINT32, &cfg.holdover, 0 },
    { "ip6_manycast", VAR_IP6, &cfg.ip6_manycast, 0 },
    { "ip_addr", VAR_IP4, &cfg.ip_addr, 0 },
    { "ip_gateway", VAR_IP4, &cfg.ip_gateway, 0 },
    { "ip_manycast", VAR_IP4, &cfg.ip_manycast, 0 },
    { "ip_netmask", VAR_IP4, &cfg.ip_netmask, 0 },
    { "loopstats_interval", VAR_UINT16, &cfg.loopstats_interval, 0},
    { "ntp_key_is_md5", VAR_FLAG, &cfg.flags, FLAG_NTPKEY_MD5 },
    { "ntp_key_is_sha1", VAR_FLAG, &cfg.flags, FLAG_NTPKEY_SHA1 },
    { "ntp_key", VAR_HEX, &cfg.ntp_key, 20 },
    { "pps_out", VAR_FLAG, &cfg.flags, FLAG_PPSEN },
    { "syslog_ip", VAR_IP4, &cfg.syslog_ip, 0 },
    { "timescale_gps", VAR_FLAG, &cfg.flags, FLAG_TIMESCALE_GPS },
    { NULL },
};



/* Command implementation */

static void
show_eeprom_error(int16_t result) {
    if (result == EERR_TIMEOUT) {
        cli_puts("ERROR: timeout while writing EEPROM\r\n");
    } else if (result == EERR_NACK) {
        cli_puts("ERROR: EEPROM is faulty or missing\r\n");
    } else if (result == EERR_FAULT) {
        cli_puts("ERROR: EEPROM is faulty\r\n");
    } else {
        cli_puts("FAIL: unable to write EEPROM\r\n");
    }
}


static void
cliWriteConfig(void) {
    int16_t result;
    if (!HAS_FEATURE(PPSEN) && (cfg.flags & FLAG_PPSEN)) {
        cli_puts("WARNING: PPS output not available on this hardware\r\n");
        cfg.flags &= ~FLAG_PPSEN;
    }
    if ((cfg.flags & (FLAG_GPSEXT | FLAG_GPSOUT)) == (FLAG_GPSEXT | FLAG_GPSOUT)) {
        cli_puts("WARNING: gps_ext_in and gps_ext_out are mutually exclusive.\r\n");
        cfg.flags &= ~FLAG_GPSOUT;
    }
    /* Check for more than one ntpkey type */
    result = 0;
    if (cfg.flags & FLAG_NTPKEY_MD5) {
        result++;
    }
    if (cfg.flags & FLAG_NTPKEY_SHA1) {
        result++;
    }
    if (result > 1) {
        cli_puts("WARNING: More than one ntpkey type specified\r\n");
        cfg.flags &= ~(FLAG_NTPKEY_MD5 | FLAG_NTPKEY_SHA1);
    }
    cli_puts("Writing EEPROM...\r\n");
    result = eeprom_write_cfg();
    if (result == EERR_OK) {
        cli_puts("OK\r\n");
        serial_drain(cl_out);
        vTaskDelay(pdMS_TO_TICKS(1000));
        NVIC_SystemReset();
    } else {
        show_eeprom_error(result);
    }
}


static void
cliDefaults(char *cmdline) {
    memset(&cfg, 0, sizeof(cfg));
    cfg.version = CFG_VERSION;
    cliWriteConfig();
}


static void
cliInfo(char *cmdline) {
    cliVersion(NULL);
    cli_print_serial();
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
    const char *bootver;
    cli_printf("Hardware:       %d.%d\r\n", hwver >> 8, hwver & 0xff);
    cli_puts(  "Software:       " VERSION "\r\n");
    bootver = info_get(boot_table, INFO_BOOTVER);
    if (bootver == NULL) {
        bootver = "no bootloader";
    }
    cli_printf("Bootloader:     %s\r\n", bootver);
}


static void
cli_cmd_fsnum(char *cmdline) {
    /* Abuse cli function to parse the hex */
    int16_t status;
    static const clivalue_t snum_value = { NULL, VAR_HEX, &snum, 8 };
    cliSetVar(&snum_value, cmdline);
    status = eeprom_write_page(0, (uint8_t*)&snum);
    if (status == EERR_OK) {
        cliInfo(NULL);
    } else {
        show_eeprom_error(status);
    }
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


#if LWIP_IPV6
static void
print_ip6addr(ip6_addr_t *addr) {
    cli_printf("%x:%x:%x:%x:%x:%x:%x:%x%s",
            IP6_ADDR_BLOCK1(addr),
            IP6_ADDR_BLOCK2(addr),
            IP6_ADDR_BLOCK3(addr),
            IP6_ADDR_BLOCK4(addr),
            IP6_ADDR_BLOCK5(addr),
            IP6_ADDR_BLOCK6(addr),
            IP6_ADDR_BLOCK7(addr),
            IP6_ADDR_BLOCK8(addr),
            ip6_addr_islinklocal(addr) ? " (link-local)" : ""
            );
}
#endif


static void
cli_print_netif(void) {
    cli_puts(    "IP:             ");
    print_ipaddr(thisif.ip_addr.addr);
    cli_puts("\r\nNetmask:        ");
    print_ipaddr(thisif.netmask.addr);
    cli_puts("\r\nGateway:        ");
    print_ipaddr(thisif.gw.addr);
#if LWIP_IPV6
    {
        int i;
        for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
            if (!ip6_addr_isvalid(netif_ip6_addr_state(&thisif, i))) {
                continue;
            }
            cli_puts("\r\nIPv6:           ");
            print_ip6addr(netif_ip6_addr(&thisif, i));
        }
    }
#endif
    cli_puts("\r\n");
}


void
cli_print_link(void) {
    char buf[SMI_DESCRIBE_SIZE];
    smi_describe_link(buf);
    cli_printf("Link status:    %s\r\n", buf);
}


static void
cli_print_serial(void) {
    uint64_t serial = 0;
    char formatted[16], *ptr;
    int i, unset = 1;
    for (i = 0; i < 6; i++) {
        if (snum.serial[i] != 0xFF) {
            unset = 0;
        }
        serial <<= 8;
        serial |= snum.serial[i];
    }
    ptr = formatted;
    if (unset) {
        strcpy(ptr, "not set!");
    } else {
        for (i = 14; i >= 0; i--) {
            formatted[i] = '0' + (serial % 10);
            serial /= 10;
            if (serial == 0) {
                ptr = &formatted[i];
                break;
            }
        }
        formatted[15] = 0;
    }
    cli_printf("Serial number:  %s\r\n", ptr);
}


void
cli_banner(void) {
    cli_puts("\r\n\r\n" BOARD_NAME "\r\n");
    cliVersion(NULL);
    cli_print_serial();
    cli_print_hwaddr();
    cli_puts("\r\nPress Enter to enable command-line\r\n");
}
