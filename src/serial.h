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

#define NO_CHAR 0xFFFF

#define USART_TX_BUF 16

typedef struct {
	USART_TypeDef *device;
	unsigned int speed;
	OS_MutexID mutex_id;

	uint8_t out_buf[USART_TX_BUF];
	queue_t out_q;

	uint16_t rx_char;
	OS_FlagID rx_flag;
} serial_t;

extern serial_t Serial1;
extern serial_t Serial4;


void serial_start(serial_t *serial, USART_TypeDef *u, int speed);
void serial_set_speed(serial_t *serial);
char serial_getc(serial_t *serial);
void serial_putc(serial_t *serial, const char value);
void serial_puts(serial_t *serial, const char *value);
void serial_write(serial_t *serial, const char *value, uint16_t size);

#endif
