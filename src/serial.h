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


typedef struct {
	USART_TypeDef		*usart;
	DMA_TypeDef			*dma;
	DMA_Channel_TypeDef	*tx_dma_ch;
	uint8_t				tx_dma_chnum;

	unsigned int speed;
	OS_MutexID mutex_id;

	uint16_t rx_char;
	OS_FlagID rx_flag;
	OS_EventID tx_sem;
} serial_t;

extern serial_t Serial1;
extern serial_t Serial4;


void serial_start(serial_t *serial, int speed);
void serial_set_speed(serial_t *serial);
void serial_puts(serial_t *serial, const char *value);
void serial_write(serial_t *serial, const char *value, uint16_t size);
void serial_printf(serial_t *serial, const char *fmt, ...);

#endif
