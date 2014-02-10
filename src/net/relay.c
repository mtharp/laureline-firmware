/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "main.h"
#include "net/relay.h"
#include "net/tcpapi.h"
#include "lwip/tcp.h"
#include "stm32/serial.h"

static uint8_t rbuf[16];
static struct tcp_pcb *relay_pcb, *relay_client;
static unsigned client_sigil, rbuf_len, needs_flush;


static void
client_err(void *arg, err_t err) {
	if ((unsigned)arg == client_sigil) {
		relay_client = NULL;
	}
}


static err_t
client_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
	if (p == NULL) {
		/* Connection closed */
		tcp_close(pcb);
		if ((unsigned)arg == client_sigil) {
			relay_client = NULL;
		}
		return ERR_OK;
	}

	if ((unsigned)arg != client_sigil) {
		pbuf_free(p);
		tcp_abort(pcb);
		return ERR_ABRT;
	}
	serial_write(gps_serial, p->payload, p->len);
	tcp_recved(pcb, p->tot_len);
	pbuf_free(p);
	return ERR_OK;
}


static err_t
relay_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
	if (relay_client != NULL) {
		tcp_abort(relay_client);
	}
	relay_client = pcb;
	tcp_arg(pcb, (void*)++client_sigil);
	tcp_err(pcb, client_err);
	tcp_recv(pcb, client_recv);
	return ERR_OK;
}


void
relay_server_start(uint16_t port) {
	relay_pcb = tcp_new();
	tcp_bind(relay_pcb, IP_ADDR_ANY, port);
	relay_pcb = tcp_listen(relay_pcb);
	tcp_accept(relay_pcb, relay_accept);
}


static void
rbuf_flush(void) {
	if (relay_client && rbuf_len != 0) {
		api_tcp_write(relay_client, rbuf, rbuf_len, TCP_WRITE_FLAG_COPY);
	}
	rbuf_len = 0;
}


void
relay_push(uint8_t val) {
	needs_flush = 1;
	if (rbuf_len < sizeof(rbuf)) {
		rbuf[rbuf_len++] = val;
	}
	if (rbuf_len == sizeof(rbuf)) {
		rbuf_flush();
	}
}


void
relay_flush(void) {
	rbuf_flush();
	if (relay_client && needs_flush) {
		api_tcp_output(relay_client);
	}
	needs_flush = 0;
}
