/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "cmdline/cmdline.h"
#include "util/parse.h"
#include <string.h>

#if CLI_TYPE_IP4 || CLI_TYPE_IP6
# include "lwip/def.h"
# include "lwip/ip_addr.h"
#endif


void
cliPrintVar(const clivalue_t *var, uint8_t full) {
    switch (var->type) {
    case VAR_UINT32:
        cli_printf("%u", *(uint32_t*)var->ptr);
        break;
    case VAR_UINT16:
        cli_printf("%u", *(uint16_t*)var->ptr);
        break;
#if CLI_TYPE_IP4
    case VAR_IP4:
        {
            ip_addr_t *ptr = (ip_addr_t*)var->ptr;
            cli_printf(IP_DIGITS_FMT, IP_DIGITS(ptr));
            break;
        }
#endif
#if CLI_TYPE_IP6
    case VAR_IP6:
        {
            ip6_addr_t *ptr = (ip6_addr_t*)var->ptr;
            cli_printf(IP6_DIGITS_FMT,
                    IP6_ADDR_BLOCK1(ptr),
                    IP6_ADDR_BLOCK2(ptr),
                    IP6_ADDR_BLOCK3(ptr),
                    IP6_ADDR_BLOCK4(ptr),
                    IP6_ADDR_BLOCK5(ptr),
                    IP6_ADDR_BLOCK6(ptr),
                    IP6_ADDR_BLOCK7(ptr),
                    IP6_ADDR_BLOCK8(ptr));
            break;
        }
#endif
#if CLI_TYPE_HEX
    case VAR_HEX:
        {
            uint8_t *ptr = (uint8_t*)var->ptr;
            int i;
            for (i = 0; i < var->len; i++) {
                cli_printf("%02x", *ptr++);
            }
            break;
        }
#endif
#if CLI_TYPE_FLAG
    case VAR_FLAG:
        if ( (*(uint32_t*)var->ptr) & var->len ) {
            cli_puts("true");
        } else {
            cli_puts("false");
        }
        break;
#endif
    case VAR_INVALID:
        break;
    }
}


void
cliSetVar(const clivalue_t *var, const char *str) {
    uint32_t val = 0;
    uint8_t val2 = 0;
    switch (var->type) {
    case VAR_UINT32:
        *(uint32_t*)var->ptr = atoi_decimal(str);
        break;
    case VAR_UINT16:
        *(uint16_t*)var->ptr = atoi_decimal(str);
        break;
#if CLI_TYPE_IP4
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
#endif
#if CLI_TYPE_IP6
    case VAR_IP6:
        {
            uint32_t *packed = (uint32_t*)var->ptr;
            uint8_t i;
            uint16_t words[8];
            for (i = 0; i < 8; i++) {
                words[i] = 0;
            }
            i = 0;
            while (*str) {
                if (*str == ':') {
                    if (++i == 8) {
                        break;
                    }
                } else {
                    words[i] = (words[i] << 4) | parse_hex(*str);
                }
                str++;
            }
            packed[0] = htonl(words[1] | ((uint32_t)words[0] << 16));
            packed[1] = htonl(words[3] | ((uint32_t)words[2] << 16));
            packed[2] = htonl(words[5] | ((uint32_t)words[4] << 16));
            packed[3] = htonl(words[7] | ((uint32_t)words[6] << 16));
            break;
        }
#endif
#if CLI_TYPE_HEX
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
            break;
        }
#endif
#if CLI_TYPE_FLAG
    case VAR_FLAG:
        if (*str == 't' || *str == 'y'
                || (*str == 'o' && *(str+1) == 'n')) {
            *(uint32_t*)var->ptr |= var->len;
        } else if (*str == 'f' || *str == 'n'
                || (*str == 'o' && *(str+1) == 'f')) {
            *(uint32_t*)var->ptr &= ~(var->len);
        }
        break;
#endif
    case VAR_INVALID:
        break;
    }
}


void
cli_cmd_set(char *cmdline) {
    uint32_t len;
    const clivalue_t *val;
    char *eqptr, *ptr2;

    len = strlen(cmdline);
    if (len == 0 || (len == 1 && cmdline[0] == '*')) {
        cli_puts("Current settings:\r\n");
        for (val = &value_table[0]; val->name != NULL; val++) {
            cli_printf("%s = ", val->name);
            cliPrintVar(val, len);
            cli_puts("\r\n");
        }
    } else if ((eqptr = strstr(cmdline, "="))) {
        /* Null-terminate the setting name */
        ptr2 = eqptr;
        *ptr2 = 0;
        while (*--ptr2 == ' ') {
            *ptr2 = 0;
        }
        eqptr++;
        len--;
        while (*eqptr == ' ') {
            eqptr++;
            len--;
        }
        for (val = &value_table[0]; val->name != NULL; val++) {
            if (!strcmp(cmdline, val->name)) {
                cliSetVar(val, eqptr);
                cli_printf("%s set to ", val->name);
                cliPrintVar(val, 0);
                return;
            }
        }
        cli_puts("ERR: Unknown variable name\r\n");
    }
}
