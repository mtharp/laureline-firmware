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

serial_t Serial1;
serial_t Serial4;



void
serial_start(serial_t *serial, USART_TypeDef *u, int speed) {
	serial->device = u;
	serial->speed = speed;
	serial->rx_char = NO_CHAR;
	if (u == USART1) {
		RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
		NVIC_SetPriority(USART1_IRQn, IRQ_PRIO_USART);
		NVIC_EnableIRQ(USART1_IRQn);
		NVIC_SetPriority(DMA1_Channel4_IRQn, IRQ_PRIO_USART);
		NVIC_EnableIRQ(DMA1_Channel4_IRQn);
		serial->dma = DMA1;
		serial->dma_channel = DMA1_Channel4;
		serial->dma_channel_num = 4;
	} else if (u == USART2) {
		RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
		NVIC_SetPriority(USART2_IRQn, IRQ_PRIO_USART);
		NVIC_EnableIRQ(USART2_IRQn);
	} else if (u == USART3) {
		RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
		NVIC_SetPriority(USART3_IRQn, IRQ_PRIO_USART);
		NVIC_EnableIRQ(USART3_IRQn);
	} else if (u == UART4) {
		RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
		NVIC_SetPriority(UART4_IRQn, IRQ_PRIO_USART);
		NVIC_EnableIRQ(UART4_IRQn);
		NVIC_SetPriority(DMA2_Channel5_IRQn, IRQ_PRIO_USART);
		NVIC_EnableIRQ(DMA2_Channel5_IRQn);
		serial->dma = DMA2;
		serial->dma_channel = DMA2_Channel5;
		serial->dma_channel_num = 5;
	} else if (u == UART5) {
		RCC->APB1ENR |= RCC_APB1ENR_UART5EN;
		NVIC_SetPriority(UART5_IRQn, IRQ_PRIO_USART);
		NVIC_EnableIRQ(UART5_IRQn);
	} else {
		HALT();
	}
	serial_set_speed(serial);
	serial->dma->IFCR = 0xF << (4 * (serial->dma_channel_num - 1));
	serial->dma_channel->CCR = 0
		| DMA_CCR1_TCIE
		| DMA_CCR1_DIR
		| DMA_CCR1_MINC
		| DMA_CCR1_PL_0
		;
	serial->dma_channel->CPAR = (uint32_t)&u->DR;
	u->CR3 = USART_CR3_DMAT;
	u->CR1 = 0
		| USART_CR1_UE
		| USART_CR1_TE
		| USART_CR1_RE
		| USART_CR1_RXNEIE
		;
	serial->mutex_id = CoCreateMutex();
	ASSERT(serial->mutex_id != E_CREATE_FAIL);
	serial->rx_flag = CoCreateFlag(1, 0);
	ASSERT(serial->rx_flag != E_CREATE_FAIL);
	serial->tx_sem = CoCreateSem(0, 1, EVENT_SORT_TYPE_FIFO);
	ASSERT(serial->tx_sem != E_CREATE_FAIL);
}


void
serial_set_speed(serial_t *serial) {
	USART_TypeDef *u = serial->device;
	if (u == USART1) {
		/* FIXME: assuming PCLK2=HCLK */
		u->BRR = system_frequency / serial->speed;
	} else {
		/* FIXME: assuming PCLK1=HCLK/2 */
		u->BRR = system_frequency / 2 / serial->speed;
	}
}


char
serial_getc(serial_t *serial) {
	char ret;
	ASSERT(CoWaitForSingleFlag(serial->rx_flag, 0) == E_OK);
	ret = (char)serial->rx_char;
	serial->rx_char = NO_CHAR;
	return ret;
}


void
serial_puts(serial_t *serial, const char *value) {
	serial_write(serial, value, strlen(value));
}


void
serial_write(serial_t *serial, const char *value, uint16_t size) {
	CoEnterMutexSection(serial->mutex_id);
	serial->dma_channel->CCR &= ~DMA_CCR1_EN;
	serial->dma_channel->CMAR = (uint32_t)value;
	serial->dma_channel->CNDTR = size;
	serial->dma_channel->CCR |= DMA_CCR1_EN;
	CoPendSem(serial->tx_sem, 0);
	CoLeaveMutexSection(serial->mutex_id);
}


static void
service_interrupt(serial_t *serial) {
	USART_TypeDef *u = serial->device;
	uint16_t sr, dr;
	sr = u->SR;
	dr = u->DR;

	if (sr & USART_SR_RXNE) {
		serial->rx_char = dr;
		isr_SetFlag(serial->rx_flag);
	}
	if (sr & USART_SR_TXE) {
		serial->device->CR1 &= ~USART_CR1_TXEIE;
		isr_PostSem(serial->tx_sem);
	}
}


static void
service_dma_interrupt(serial_t *serial) {
	serial->dma->IFCR = 0xF << (4 * (serial->dma_channel_num - 1));
	serial->dma_channel->CCR &= ~DMA_CCR1_EN;
	serial->device->CR1 |= USART_CR1_TXEIE;
}


void
USART1_IRQHandler(void) {
	CoEnterISR();
	service_interrupt(&Serial1);
	CoExitISR();
}


void
DMA1_Channel4_IRQHandler(void) {
	CoEnterISR();
	service_dma_interrupt(&Serial1);
	CoExitISR();
}


void
UART4_IRQHandler(void) {
	CoEnterISR();
	service_interrupt(&Serial4);
	CoExitISR();
}


void
DMA2_Channel5_IRQHandler(void) {
	CoEnterISR();
	service_dma_interrupt(&Serial4);
	CoExitISR();
}
