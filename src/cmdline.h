/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _CMDLINE_H
#define _CMDLINE_H

#include "serial.h"

#define no_cli_printf(...) if (!cl_enabled) { cli_printf(__VA_ARGS__); }

extern uint8_t cl_enabled;

void cli_set_output(serial_t *output);
void cli_printf(const char *fmt, ...);
void cli_puts(const char *value);
void cli_banner(void);
void cli_feed(char c);


#endif
