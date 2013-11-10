/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "tcpapi.h"

OS_FlagID api_flag; /* Wakes tcpip thread for a call */
static OS_MutexID api_mutex; /* Serializes client calls */
static OS_EventID api_sem; /* Wakes client for return value */
static tcpapi_msg_t msg;

void
api_start(void) {
	ASSERT((api_flag = CoCreateFlag(1, 0)) != E_CREATE_FAIL);
	ASSERT((api_mutex = CoCreateMutex()) != E_CREATE_FAIL);
	ASSERT((api_sem = CoCreateSem(0, 1, EVENT_SORT_TYPE_FIFO)) != E_CREATE_FAIL);
}


void
api_accept(void) {
	if (msg.func == NULL) {
		return;
	}
	msg.func(&msg);
	msg.func = NULL;
	CoPostSem(api_sem);
}


static void
do_tcp_write(tcpapi_msg_t *msg) {
	msg->ret = tcp_write(msg->msg.wr.pcb, msg->msg.wr.data, msg->msg.wr.len,
			msg->msg.wr.flags);
}


err_t
api_tcp_write(struct tcp_pcb *pcb, void *data, uint16_t len, uint8_t flags) {
	err_t ret;
	CoEnterMutexSection(api_mutex);
	msg.msg.wr.pcb = pcb;
	msg.msg.wr.data = data;
	msg.msg.wr.len = len;
	msg.msg.wr.flags = flags;
	msg.func = do_tcp_write;
	CoSetFlag(api_flag);
	CoPendSem(api_sem, 0);
	ret = msg.ret;
	CoLeaveMutexSection(api_mutex);
	return ret;
}


static void
do_tcp_output(tcpapi_msg_t *msg) {
	msg->ret = tcp_output(msg->msg.wr.pcb);
}


err_t
api_tcp_output(struct tcp_pcb *pcb) {
	err_t ret;
	CoEnterMutexSection(api_mutex);
	msg.msg.wr.pcb = pcb;
	msg.func = do_tcp_output;
	CoSetFlag(api_flag);
	CoPendSem(api_sem, 0);
	ret = msg.ret;
	CoLeaveMutexSection(api_mutex);
	return ret;
}
