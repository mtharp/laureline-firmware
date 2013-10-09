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
#include "cmdline/cmdline.h"
#include "util/parse.h"


uint8_t cl_enabled;
serial_t *cl_out;

static char cl_buf[64];
static uint8_t cl_count;


/* Initialization and UART handling */

void
cli_set_output(serial_t *output) {
	cl_out = output;
}


/* Command-line helpers */

static void
cli_prompt(void) {
	cl_count = 0;
	cl_enabled = 1;
	cli_puts("\r\n# ");
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
		cli_cmd_exit(cl_buf);
	} else if (c == 12) {
		/* Clear screen */
		cli_puts("\033[2J\033[1;1H");
		cli_prompt();
	} else if (c == '\n' || c == '\r') {
		if (cl_count) {
			/* Enter pressed */
			int i;
			char *cmd;
			cli_puts("\r\n");
			cl_buf[cl_count] = 0;
			cmd = strtok_s(cl_buf, ' ');
			for (i = 0; cmd_table[i].name != NULL; i++) {
				if (strcasecmp(cmd, cmd_table[i].name) == 0) {
					/* Rest of the cmdline */
					cmd = strtok_s(NULL, 0);
					if (cmd == NULL) {
						cmd = "";
					}
					cmd_table[i].func(cmd);
					break;
				}
			}
			if (cmd_table[i].name == NULL) {
				cli_puts("ERR: Unknown command, try 'help'\r\n");
			}
			memset(cl_buf, 0, sizeof(cl_buf));
		} else if (c == '\n' && cl_buf[0] == '\r') {
			/* Ignore \n after \r */
			return;
		}
		if (cl_enabled) {
			cli_prompt();
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


/* Core commands */


void
cli_cmd_help(char *cmdline) {
	int i;
	cli_puts("Available commands:\r\n");
	for (i = 0; cmd_table[i].name != NULL; i++) {
		cli_printf("%s\t%s\r\n", cmd_table[i].name, cmd_table[i].param);
	}
}


void
cli_cmd_exit(char *cmdline) {
	cl_enabled = 0;
	cli_puts("Exiting cmdline mode.\r\n"
			"Configuration changes have not been saved.\r\n"
			"Press Enter to enable cmdline.\r\n");
}
