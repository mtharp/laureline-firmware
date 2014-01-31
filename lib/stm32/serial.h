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
#include "util/queue.h"


typedef struct {
	USART_TypeDef	*usart;
	unsigned int	speed;
	OS_MutexID		mutex_id;
	queue_t			tx_q;
	uint8_t			tx_buf[16];
	queue_t			rx_q;
	uint8_t			rx_buf[16];
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


void serial_start(serial_t *serial, int speed);
void serial_set_speed(serial_t *serial);
void serial_puts(serial_t *serial, const char *value);
void serial_write(serial_t *serial, const char *value, uint16_t size);
void serial_printf(serial_t *serial, const char *fmt, ...);
void serial_drain(serial_t *serial);
int16_t serial_get(serial_t *serial, int timeout);

#endif
