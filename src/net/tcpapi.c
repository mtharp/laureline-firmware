/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "logging.h"
#include "net/tcpapi.h"
#include "lwip/dns.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include <string.h>

OS_FlagID api_flag; /* Wakes tcpip thread for a call */
static OS_MutexID api_mutex; /* Serializes client calls */
static OS_EventID api_sem; /* Wakes client for return value */
static OS_TID api_thread;
static tcpapi_msg_t msg;

void
api_start(void) {
	ASSERT((api_flag = CoCreateFlag(1, 0)) != E_CREATE_FAIL);
	ASSERT((api_mutex = CoCreateMutex()) != E_CREATE_FAIL);
	ASSERT((api_sem = CoCreateSem(0, 1, EVENT_SORT_TYPE_FIFO)) != E_CREATE_FAIL);
	api_thread = E_CREATE_FAIL;
}


void
api_set_main_thread(OS_TID thread) {
	api_thread = thread;
}


void
api_accept(void) {
	if (msg.func == NULL) {
		return;
	}
	msg.ret = msg.func(&msg);
	if (msg.ret != ERR_INPROGRESS) {
		msg.func = NULL;
		CoPostSem(api_sem);
	}
}


static err_t
api_call(api_func func) {
	err_t ret;
	msg.func = func;
	if (CoGetCurTaskID() == api_thread) {
		func(&msg);
		msg.func = NULL;
	} else {
		CoSetFlag(api_flag);
		CoPendSem(api_sem, 0);
	}
	ret = msg.ret;
	CoLeaveMutexSection(api_mutex);
	return ret;
}


static err_t
do_tcp_write(tcpapi_msg_t *msg) {
	return tcp_write(msg->msg.wr.pcb, msg->msg.wr.data, msg->msg.wr.len,
			msg->msg.wr.flags);
}


err_t
api_tcp_write(struct tcp_pcb *pcb, const void *data, uint16_t len, uint8_t flags) {
	CoEnterMutexSection(api_mutex);
	msg.msg.wr.pcb = pcb;
	msg.msg.wr.data = data;
	msg.msg.wr.len = len;
	msg.msg.wr.flags = flags;
	return api_call(do_tcp_write);
}


static err_t
do_tcp_output(tcpapi_msg_t *msg) {
	return tcp_output(msg->msg.wr.pcb);
}


err_t
api_tcp_output(struct tcp_pcb *pcb) {
	CoEnterMutexSection(api_mutex);
	msg.msg.wr.pcb = pcb;
	return api_call(do_tcp_output);
}


static err_t
do_udp_connect(tcpapi_msg_t *msg) {
	return udp_connect(msg->msg.uconn.pcb, msg->msg.uconn.addr,
			msg->msg.uconn.port);
}

err_t
api_udp_connect(struct udp_pcb *pcb, ip_addr_t *addr, uint16_t port) {
	CoEnterMutexSection(api_mutex);
	msg.msg.uconn.pcb = pcb;
	msg.msg.uconn.addr = addr;
	msg.msg.uconn.port = port;
	return api_call(do_udp_connect);
}


static err_t
do_udp_send(tcpapi_msg_t *msg) {
	struct pbuf *p;
	err_t ret;
	p = pbuf_alloc(PBUF_TRANSPORT, msg->msg.usend.len, PBUF_RAM);
	if (p == NULL) {
		return ERR_MEM;
	}
	memcpy(p->payload, msg->msg.usend.data, msg->msg.usend.len);
	ret = udp_send(msg->msg.usend.pcb, p);
	pbuf_free(p);
	return ret;
}

err_t
api_udp_send(struct udp_pcb *pcb, const void *data, uint16_t len) {
	CoEnterMutexSection(api_mutex);
	msg.msg.usend.pcb = pcb;
	msg.msg.usend.data = data;
	msg.msg.usend.len = len;
	return api_call(do_udp_send);
}


static void
got_udp(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, uint16_t port) {
	tcpapi_msg_t *msg = (tcpapi_msg_t*)arg;
	udp_recv(pcb, NULL, NULL);
	*(msg->msg.urecv.len) = pbuf_copy_partial(p,
			msg->msg.urecv.data, *(msg->msg.urecv.len), 0);
	msg->ret = ERR_OK;
	msg->func = NULL;
	pbuf_free(p);
	CoPostSem(api_sem);
}


static err_t
do_udp_recv(tcpapi_msg_t *msg) {
	udp_recv(msg->msg.urecv.pcb, got_udp, msg);
	return ERR_INPROGRESS;
}


err_t
api_udp_recv(struct udp_pcb *pcb, void *data, uint16_t *len) {
	CoEnterMutexSection(api_mutex);
	msg.msg.urecv.pcb = pcb;
	msg.msg.urecv.data = data;
	msg.msg.urecv.len = len;
	return api_call(do_udp_recv);
}


static void
gethost_callback(const char *name, ip_addr_t *addr, void *arg) {
	tcpapi_msg_t *msg = (tcpapi_msg_t*)arg;
	if (addr == NULL) {
		msg->ret = ERR_VAL;
	} else {
		msg->ret = ERR_OK;
		ip_addr_set(msg->msg.gh.addr, addr);
	}
	msg->func = NULL;
	CoPostSem(api_sem);
}



static err_t
do_gethostbyname(tcpapi_msg_t *msg) {
	return dns_gethostbyname(msg->msg.gh.name, msg->msg.gh.addr,
			gethost_callback, (void*)msg);
}


err_t
api_gethostbyname(const char *name, ip_addr_t *addr) {
	CoEnterMutexSection(api_mutex);
	msg.msg.gh.name = name;
	msg.msg.gh.addr = addr;
	return api_call(do_gethostbyname);
}
