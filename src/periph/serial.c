/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */


#include "common.h"
#include "init.h"
#include "periph/serial.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#if USE_SERIAL_USART1
serial_t Serial1;
#endif
#if USE_SERIAL_UART4
serial_t Serial4;
#endif

static void serial_outq_cb(void *arg);


void
serial_start(serial_t *serial, int speed) {
	IRQn_Type irqn = 0;
	queue_init(&serial->tx_q, serial->tx_buf, sizeof(serial->tx_buf));
	queue_cb(&serial->tx_q, &serial_outq_cb, serial);
	queue_init(&serial->rx_q, serial->rx_buf, sizeof(serial->rx_buf));
	serial->speed = speed;
	if (serial == &Serial1) {
		RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
		irqn = USART1_IRQn;
		serial->usart = USART1;
#if 0
	} else if (serial == &Serial2) {
		RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
		irqn = USART2_IRQn;
		serial->usart = USART2;
	} else if (serial == &Serial3) {
		RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
		irqn = USART3_IRQn;
		serial->usart = USART3;
#endif
	} else if (serial == &Serial4) {
		RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
		irqn = UART4_IRQn;
		serial->usart = UART4;
#if 0
	} else if (serial == &Serial5) {
		RCC->APB1ENR |= RCC_APB1ENR_UART5EN;
		irqn = UART5_IRQn;
		serial->usart = UART5;
#endif
	} else {
		HALT();
	}
	NVIC_SetPriority(irqn, IRQ_PRIO_USART);
	NVIC_EnableIRQ(irqn);
	serial_set_speed(serial);
	serial->usart->CR1 = 0
		| USART_CR1_UE
		| USART_CR1_TE
		| USART_CR1_RE
		| USART_CR1_RXNEIE
		| USART_CR1_IDLEIE
		;
	serial->mutex_id = CoCreateMutex();
	ASSERT(serial->mutex_id != E_CREATE_FAIL);
}


void
serial_set_speed(serial_t *serial) {
	USART_TypeDef *u = serial->usart;
	if (u == USART1) {
		/* FIXME: assuming PCLK2=HCLK */
		u->BRR = system_frequency / serial->speed;
	} else {
		/* FIXME: assuming PCLK1=HCLK/2 */
		u->BRR = system_frequency / 2 / serial->speed;
	}
}


void
serial_puts(serial_t *serial, const char *value) {
	CoEnterMutexSection(serial->mutex_id);
	outqueue_put(&serial->tx_q, (const uint8_t*)value, strlen(value), TIMEOUT_FOREVER);
	CoLeaveMutexSection(serial->mutex_id);
}


void
serial_write(serial_t *serial, const char *value, uint16_t size) {
	CoEnterMutexSection(serial->mutex_id);
	outqueue_put(&serial->tx_q, (const uint8_t*)value, size, TIMEOUT_FOREVER);
	CoLeaveMutexSection(serial->mutex_id);
}


void
serial_printf(serial_t *serial, const char *fmt, ...) {
	static char fmt_buf[64];
	va_list ap;
	va_start(ap, fmt);
	CoEnterMutexSection(serial->mutex_id);
	if (vsnprintf(fmt_buf, sizeof(fmt_buf), fmt, ap) >= 0) {
		outqueue_put(&serial->tx_q, (const uint8_t*)fmt_buf, strlen(fmt_buf), TIMEOUT_FOREVER);
	}
	va_end(ap);
	CoLeaveMutexSection(serial->mutex_id);
}


int16_t
serial_get(serial_t *serial, int timeout) {
	return inqueue_get(&serial->rx_q, timeout);
}


static void
serial_outq_cb(void *arg) {
	serial_t *serial = (serial_t*)arg;
	serial->usart->CR1 |= USART_CR1_TXEIE;
}


static void
service_interrupt(serial_t *serial) {
	USART_TypeDef *u = serial->usart;
	uint16_t sr, dr;
	int16_t val;
	sr = u->SR;
	dr = u->DR;

	if (sr & USART_SR_RXNE) {
		inqueue_putI(&serial->rx_q, dr);
	}
	if (sr & USART_SR_IDLE) {
		inqueue_flushI(&serial->rx_q);
	}
	if (sr & USART_SR_TXE) {
		if ((val = outqueue_getI(&serial->tx_q)) < 0) {
			u->CR1 &= ~USART_CR1_TXEIE;
		} else {
			u->DR = val;
		}
	}
}


#if USE_SERIAL_USART1
void
USART1_IRQHandler(void) {
	CoEnterISR();
	service_interrupt(&Serial1);
	CoExitISR();
}
#endif


#if USE_SERIAL_UART4
void
UART4_IRQHandler(void) {
	CoEnterISR();
	service_interrupt(&Serial4);
	CoExitISR();
}
#endif
