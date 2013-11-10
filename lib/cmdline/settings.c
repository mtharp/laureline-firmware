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

#ifdef CLI_TYPE_IP4
# include "lwip/def.h"
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
	case VAR_BOOL:
		cli_printf("%u", !!*(uint8_t*)var->ptr);
		break;
#ifdef CLI_TYPE_IP4
	case VAR_IP4:
		{
			uint8_t *addr = (uint8_t*)var->ptr;
			cli_printf("%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
			break;
		}
#endif
#ifdef CLI_TYPE_HEX
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
	case _:
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
	case VAR_BOOL:
		*(uint8_t*)var->ptr = !!atoi_decimal(str);
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
		}
#endif
	case _:
		break;
	}
}


void
cli_cmd_set(char *cmdline) {
	uint32_t i, len;
	const clivalue_t *val;
	char *eqptr = NULL;

	len = strlen(cmdline);
	if (len == 0 || (len == 1 && cmdline[0] == '*')) {
		cli_puts("Current settings:\r\n");
		for (i = 0; value_table[i].name != NULL; i++) {
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
		for (i = 0; value_table[i].name != NULL; i++) {
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
