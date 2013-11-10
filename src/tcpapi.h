/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _TCPAPI_H
#define _TCPAPI_H

#include "lwip/tcp.h"

struct tcpapi_msg;

typedef void (*api_func)(struct tcpapi_msg *msg);

typedef struct tcpapi_msg {
	api_func func;
	err_t ret;
	union {
		/* api_write */
		struct {
			struct tcp_pcb *pcb;
			void *data;
			uint16_t len;
			uint8_t flags;
		} wr;
	} msg;
} tcpapi_msg_t;


void api_start(void);
void api_accept(void);

err_t api_tcp_write(struct tcp_pcb *pcb, void *data, uint16_t len, uint8_t flags);
err_t api_tcp_output(struct tcp_pcb *pcb);

extern OS_FlagID api_flag;

#endif
