/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _CMDLINE_CMDLINE_H
#define _CMDLINE_CMDLINE_H

#include "stm32/serial.h"

#define cli_puts(val) do { serial_puts(cl_out, val); } while (0)
#define cli_printf(...) do { serial_printf(cl_out, __VA_ARGS__); } while (0)
#define idle_puts(val) do { if (!cl_enabled) { serial_puts(cl_out, val); } } while (0)
#define idle_printf(...) do { if (!cl_enabled) { serial_printf(cl_out, __VA_ARGS__); } } while (0)

typedef struct {
    char *name;
    char *param;
    void (*func)(char *cmdline);
} clicmd_t;


extern uint8_t cl_enabled;
extern serial_t *cl_out;
extern const clicmd_t cmd_table[];

void cli_feed(char c);
void cli_set_output(serial_t *output);

void cli_cmd_help(char *cmdline);
void cli_cmd_exit(char *cmdline);

#include "cmdline/settings.h"

#endif
