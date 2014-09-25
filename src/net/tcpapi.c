/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "task.h"

#include "logging.h"
#include "net/tcpapi.h"
#include "net/tcpqueue.h"
#include "lwip/dns.h"
#include "lwip/tcp.h"
#include "lwip/timers.h"
#include "lwip/udp.h"
#include <string.h>

static TaskHandle_t api_thread;
static SemaphoreHandle_t api_mutex;
static tcpapi_sems_t *semlist;

void
api_start(void) {
    ASSERT((api_mutex = xSemaphoreCreateMutex()));
    semlist = NULL;
    api_thread = NULL;
}


void
api_set_main_thread(TaskHandle_t thread) {
    api_thread = thread;
}


void
api_accept(void *p) {
    tcpapi_msg_t *msg = (tcpapi_msg_t*)p;
    msg->ret = msg->func(msg);
    if (msg->ret != ERR_INPROGRESS) {
        xSemaphoreGive(msg->sem);
    }
}


static err_t
api_call(tcpapi_msg_t *msg, api_func func) {
    tcpapi_sems_t *semptr;
    TaskHandle_t tid = xTaskGetCurrentTaskHandle();
    ASSERT(api_thread);
    if (tid == api_thread) {
        /* Run inline */
        err_t ret = func(msg);
        ASSERT(ret != ERR_INPROGRESS); /* can't sleep in api thread */
        return ret;
    }
    msg->func = func;
    xSemaphoreTake(api_mutex, portMAX_DELAY);
    /* Use semaphore for the caller's thread, creating first if needed */
    msg->sem = NULL;
    for (semptr = semlist; semptr; semptr = semptr->next) {
        if (semptr->thread == tid) {
            msg->sem = semptr->sem;
            break;
        }
    }
    if (msg->sem == NULL) {
        ASSERT((semptr = malloc(sizeof(*semptr))));
        ASSERT((msg->sem = semptr->sem = xSemaphoreCreateBinary()));
        semptr->thread = tid;
        semptr->next = semlist;
        semlist = semptr;
    }
    xSemaphoreGive(api_mutex);
    /* Post message to tcpip thread */
    xQueueSend(tcpip_queue, &msg, portMAX_DELAY);
    /* Sleep until result is ready */
    xSemaphoreTake(msg->sem, portMAX_DELAY);
    return msg->ret;
}


/*
 * api_tcp_write
 */

static err_t
do_tcp_write(tcpapi_msg_t *msg) {
    return tcp_write(msg->msg.wr.pcb, msg->msg.wr.data, msg->msg.wr.len,
            msg->msg.wr.flags);
}


err_t
api_tcp_write(struct tcp_pcb *pcb, const void *data, uint16_t len, uint8_t flags) {
    tcpapi_msg_t msg;
    msg.msg.wr.pcb = pcb;
    msg.msg.wr.data = data;
    msg.msg.wr.len = len;
    msg.msg.wr.flags = flags;
    return api_call(&msg, do_tcp_write);
}


/*
 * api_tcp_output
 */

static err_t
do_tcp_output(tcpapi_msg_t *msg) {
    return tcp_output(msg->msg.wr.pcb);
}


err_t
api_tcp_output(struct tcp_pcb *pcb) {
    tcpapi_msg_t msg;
    msg.msg.wr.pcb = pcb;
    return api_call(&msg, do_tcp_output);
}


/*
 * api_udp_connect
 */

static err_t
do_udp_connect(tcpapi_msg_t *msg) {
    return udp_connect(msg->msg.uconn.pcb, msg->msg.uconn.addr,
            msg->msg.uconn.port);
}

err_t
api_udp_connect(struct udp_pcb *pcb, ip_addr_t *addr, uint16_t port) {
    tcpapi_msg_t msg;
    msg.msg.uconn.pcb = pcb;
    msg.msg.uconn.addr = addr;
    msg.msg.uconn.port = port;
    return api_call(&msg, do_udp_connect);
}


/*
 * api_udp_send
 */

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
    tcpapi_msg_t msg;
    msg.msg.usend.pcb = pcb;
    msg.msg.usend.data = data;
    msg.msg.usend.len = len;
    return api_call(&msg, do_udp_send);
}


/*
 * api_udp_recv
 */

static void
got_udp_timedout(void *arg) {
    tcpapi_msg_t *msg = (tcpapi_msg_t*)arg;
    udp_recv(msg->msg.urecv.pcb, NULL, NULL);
    msg->ret = ERR_TIMEOUT;
    xSemaphoreGive(msg->sem);
}


static void
got_udp(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, uint16_t port) {
    tcpapi_msg_t *msg = (tcpapi_msg_t*)arg;
    udp_recv(pcb, NULL, NULL);
    sys_untimeout(got_udp_timedout, msg);
    *(msg->msg.urecv.len) = pbuf_copy_partial(p,
            msg->msg.urecv.data, *(msg->msg.urecv.len), 0);
    msg->ret = ERR_OK;
    pbuf_free(p);
    xSemaphoreGive(msg->sem);
}


static err_t
do_udp_recv(tcpapi_msg_t *msg) {
    udp_recv(msg->msg.urecv.pcb, got_udp, msg);
    if (msg->timeout) {
        sys_timeout(msg->timeout, got_udp_timedout, msg);
    }
    return ERR_INPROGRESS;
}


err_t
api_udp_recv(struct udp_pcb *pcb, void *data, uint16_t *len, uint16_t timeout) {
    tcpapi_msg_t msg;
    msg.timeout = timeout;
    msg.msg.urecv.pcb = pcb;
    msg.msg.urecv.data = data;
    msg.msg.urecv.len = len;
    return api_call(&msg, do_udp_recv);
}


/*
 * api_gethostbyname
 */

static void
gethost_callback(const char *name, ip_addr_t *addr, void *arg) {
    tcpapi_msg_t *msg = (tcpapi_msg_t*)arg;
    if (addr == NULL) {
        msg->ret = ERR_VAL;
    } else {
        msg->ret = ERR_OK;
        ip_addr_set(msg->msg.gh.addr, addr);
    }
    xSemaphoreGive(msg->sem);
}



static err_t
do_gethostbyname(tcpapi_msg_t *msg) {
    return dns_gethostbyname(msg->msg.gh.name, msg->msg.gh.addr,
            gethost_callback, (void*)msg);
}


err_t
api_gethostbyname(const char *name, ip_addr_t *addr) {
    tcpapi_msg_t msg;
    msg.msg.gh.name = name;
    msg.msg.gh.addr = addr;
    return api_call(&msg, do_gethostbyname);
}
