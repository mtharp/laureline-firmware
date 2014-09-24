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
#include "task.h"

struct tcpapi_msg;

typedef err_t (*api_func)(struct tcpapi_msg *msg);

typedef struct tcpapi_msg {
    api_func func;
    SemaphoreHandle_t sem;
    err_t ret;
    uint16_t timeout;
    union {
        /* api_tcp_write */
        struct {
            struct tcp_pcb *pcb;
            const void *data;
            uint16_t len;
            uint8_t flags;
        } wr;
        /* api_udp_connect */
        struct {
            struct udp_pcb *pcb;
            ip_addr_t *addr;
            uint16_t port;
        } uconn;
        /* api_udp_send */
        struct {
            struct udp_pcb *pcb;
            const void *data;
            uint16_t len;
        } usend;
        /* api_udp_recv */
        struct {
            struct udp_pcb *pcb;
            void *data;
            uint16_t *len;
        } urecv;
        /* api_gethostbyname */
        struct {
            const char *name;
            ip_addr_t *addr;
        } gh;
    } msg;
} tcpapi_msg_t;


typedef struct tcpapi_sems {
    TaskHandle_t thread;
    SemaphoreHandle_t sem;
    struct tcpapi_sems *next;
} tcpapi_sems_t;


void api_start(void);
void api_set_main_thread(TaskHandle_t thread);
void api_accept(void *p);

err_t api_tcp_write(struct tcp_pcb *pcb, const void *data, uint16_t len, uint8_t flags);
err_t api_tcp_output(struct tcp_pcb *pcb);
err_t api_udp_connect(struct udp_pcb *pcb, ip_addr_t *addr, uint16_t port);
err_t api_udp_send(struct udp_pcb *pcb, const void *data, uint16_t len);
err_t api_udp_recv(struct udp_pcb *pcb, void *data, uint16_t *len, uint16_t timeout);
err_t api_gethostbyname(const char *name, ip_addr_t *addr);

#endif
