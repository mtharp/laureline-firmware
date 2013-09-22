/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */


#include "common.h"
#include "init.h"
#include "serial.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

serial_t Serial1;
serial_t Serial4;

static void serial_outq_cb(void *arg);


void
serial_start(serial_t *serial, int speed) {
	outqueue_init(&serial->tx_q, serial->tx_buf, sizeof(serial->tx_buf));
	outqueue_cb(&serial->tx_q, &serial_outq_cb, serial);
	serial->speed = speed;
	serial->rx_char = NO_CHAR;
	if (serial == &Serial1) {
		RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
		NVIC_SetPriority(USART1_IRQn, IRQ_PRIO_USART);
		NVIC_EnableIRQ(USART1_IRQn);
		NVIC_SetPriority(DMA1_Channel4_IRQn, IRQ_PRIO_USART);
		NVIC_EnableIRQ(DMA1_Channel4_IRQn);
		serial->usart = USART1;
		serial->dma = DMA1;
		//serial->tx_dma_ch = DMA1_Channel4;
		//serial->tx_dma_chnum = 4;
#if 0
	} else if (u == USART2) {
		RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
		NVIC_SetPriority(USART2_IRQn, IRQ_PRIO_USART);
		NVIC_EnableIRQ(USART2_IRQn);
	} else if (u == USART3) {
		RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
		NVIC_SetPriority(USART3_IRQn, IRQ_PRIO_USART);
		NVIC_EnableIRQ(USART3_IRQn);
#endif
	} else if (serial == &Serial4) {
		RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
		NVIC_SetPriority(UART4_IRQn, IRQ_PRIO_USART);
		NVIC_EnableIRQ(UART4_IRQn);
		NVIC_SetPriority(DMA2_Channel5_IRQn, IRQ_PRIO_USART);
		NVIC_EnableIRQ(DMA2_Channel5_IRQn);
		serial->usart = UART4;
		serial->dma = DMA2;
		//serial->tx_dma_ch = DMA2_Channel5;
		//serial->tx_dma_chnum = 5;
#if 0
	} else if (u == UART5) {
		RCC->APB1ENR |= RCC_APB1ENR_UART5EN;
		NVIC_SetPriority(UART5_IRQn, IRQ_PRIO_USART);
		NVIC_EnableIRQ(UART5_IRQn);
#endif
	} else {
		HALT();
	}
	serial_set_speed(serial);
	/*serial->dma->IFCR = 0xF << (4 * (serial->tx_dma_chnum - 1));
	serial->tx_dma_ch->CCR = 0
		| DMA_CCR1_TCIE
		| DMA_CCR1_DIR
		| DMA_CCR1_MINC
		| DMA_CCR1_PL_0
		;
	serial->tx_dma_ch->CPAR = (uint32_t)&serial->usart->DR;
	serial->usart->CR3 = USART_CR3_DMAT;*/
	serial->usart->CR1 = 0
		| USART_CR1_UE
		| USART_CR1_TE
		| USART_CR1_RE
		| USART_CR1_RXNEIE
		;
	serial->mutex_id = CoCreateMutex();
	ASSERT(serial->mutex_id != E_CREATE_FAIL);
	serial->rx_flag = CoCreateFlag(1, 0);
	ASSERT(serial->rx_flag != E_CREATE_FAIL);
	/*serial->tx_sem = CoCreateSem(0, 1, EVENT_SORT_TYPE_FIFO);
	ASSERT(serial->tx_sem != E_CREATE_FAIL);*/
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
	outqueue_put(&serial->tx_q, (const uint8_t*)value, strlen(value), 0);
	CoLeaveMutexSection(serial->mutex_id);
}


void
serial_write(serial_t *serial, const char *value, uint16_t size) {
	CoEnterMutexSection(serial->mutex_id);
	outqueue_put(&serial->tx_q, (const uint8_t*)value, size, 0);
	CoLeaveMutexSection(serial->mutex_id);
}


static void
serial_outq_cb(void *arg) {
	serial_t *serial = (serial_t*)arg;
	serial->usart->CR1 |= USART_CR1_TXEIE;
}


void
serial_printf(serial_t *serial, const char *fmt, ...) {
	static char fmt_buf[64];
	va_list ap;
	va_start(ap, fmt);
	CoEnterMutexSection(serial->mutex_id);
	if (vsnprintf(fmt_buf, sizeof(fmt_buf), fmt, ap) >= 0) {
		outqueue_put(&serial->tx_q, (const uint8_t*)fmt_buf, strlen(fmt_buf), 0);
	}
	va_end(ap);
	CoLeaveMutexSection(serial->mutex_id);
}



static void
service_interrupt(serial_t *serial) {
	USART_TypeDef *u = serial->usart;
	uint16_t sr, dr;
	int16_t val;
	sr = u->SR;
	dr = u->DR;

	if (sr & USART_SR_RXNE) {
		serial->rx_char = dr;
		isr_SetFlag(serial->rx_flag);
	}
	if (sr & USART_SR_TXE) {
		if ((val = outqueue_getI(&serial->tx_q)) < 0) {
			u->CR1 &= ~USART_CR1_TXEIE;
		} else {
			u->DR = val;
		}
	}
}


/*
static void
service_dma_interrupt(serial_t *serial) {
	serial->dma->IFCR = 0xF << (4 * (serial->tx_dma_chnum - 1));
	serial->tx_dma_ch->CCR &= ~DMA_CCR1_EN;
	serial->usart->CR1 |= USART_CR1_TXEIE;
}
*/


void
USART1_IRQHandler(void) {
	CoEnterISR();
	service_interrupt(&Serial1);
	CoExitISR();
}


/*
void
DMA1_Channel4_IRQHandler(void) {
	CoEnterISR();
	service_dma_interrupt(&Serial1);
	CoExitISR();
}
*/


void
UART4_IRQHandler(void) {
	CoEnterISR();
	service_interrupt(&Serial4);
	CoExitISR();
}


/*
void
DMA2_Channel5_IRQHandler(void) {
	CoEnterISR();
	service_dma_interrupt(&Serial4);
	CoExitISR();
}
*/
