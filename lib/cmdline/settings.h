/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _CMDLINE_SETTINGS_H
#define _CMDLINE_SETTINGS_H

#include <stdint.h>

#define CLI_TYPE_IP4 1
#define CLI_TYPE_IP6 1
#define CLI_TYPE_HEX 1
#define CLI_TYPE_FLAG 1

#define IP_DIGITS(x) ((x)->addr) & 0xff, ((x)->addr >> 8) & 0xff, \
    ((x)->addr >> 16) & 0xff, ((x)->addr >> 24) & 0xff
#define IP_DIGITS_FMT "%lu.%lu.%lu.%lu"
#define IP6_DIGITS_FMT "%x:%x:%x:%x:%x:%x:%x:%x"


typedef enum {
    VAR_UINT32,
    VAR_UINT16,
#if CLI_TYPE_IP4
    VAR_IP4,
#endif
#if CLI_TYPE_IP6
    VAR_IP6,
#endif
#if CLI_TYPE_HEX
    VAR_HEX,
#endif
#if CLI_TYPE_FLAG
    VAR_FLAG,
#endif
    VAR_INVALID
} vartype_e;


typedef struct {
    const char *name;
    const vartype_e type;
    void *ptr;
    int len;
} clivalue_t;


extern const clivalue_t value_table[];

void cli_cmd_set(char *cmdline);

void cliPrintVar(const clivalue_t *var, uint8_t full);
void cliSetVar(const clivalue_t *var, const char *str);


#endif
