/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */


#ifndef _SERIAL_H
#define _SERIAL_H

#include "common.h"
#include "queue.h"
#include "semphr.h"
#include "stm32/dma.h"

#define SERIAL_TX_SIZE  16
#define SERIAL_RX_SIZE  16


typedef struct {
    USART_TypeDef       *usart;
    unsigned int        speed;
    SemaphoreHandle_t   mutex;
    /* non-DMA */
    QueueHandle_t       tx_q;
    QueueHandle_t       rx_q;
    /* DMA */
    SemaphoreHandle_t   tcie_sem;
    const dma_ch_t      *tx_dma;
} serial_t;

#if USE_SERIAL_USART1
extern serial_t Serial1;
#endif
#if USE_SERIAL_UART4
extern serial_t Serial4;
#endif
#if USE_SERIAL_UART5
extern serial_t Serial5;
#endif


void serial_start(serial_t *serial, int speed
#if configUSE_QUEUE_SETS
        , QueueSetHandle_t queue_set
#endif
        );
void serial_set_speed(serial_t *serial);
void serial_puts(serial_t *serial, const char *value);
void serial_write(serial_t *serial, const char *value, uint16_t size);
void serial_printf(serial_t *serial, const char *fmt, ...);
void serial_drain(serial_t *serial);
int16_t serial_get(serial_t *serial, TickType_t timeout);

#endif
