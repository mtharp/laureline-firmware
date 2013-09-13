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
	} else if (u == UART5) {
		RCC->APB1ENR |= RCC_APB1ENR_UART5EN;
		NVIC_SetPriority(UART5_IRQn, IRQ_PRIO_USART);
		NVIC_EnableIRQ(UART5_IRQn);
	} else {
		HALT();
	}
	serial_set_speed(serial);
	u->CR1 = 0
		| USART_CR1_UE
		| USART_CR1_TE
		| USART_CR1_RE
		| USART_CR1_RXNEIE
		;
	serial->mutex_id = CoCreateMutex();
	if (serial->mutex_id == E_CREATE_FAIL) { HALT(); }
	serial->tx_flag = CoCreateFlag(1, 0);
	if (serial->tx_flag == E_CREATE_FAIL) { HALT(); }
	serial->rx_flag = CoCreateFlag(1, 0);
	if (serial->rx_flag == E_CREATE_FAIL) { HALT(); }
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
	if (CoWaitForSingleFlag(serial->rx_flag, 0) != E_OK) {
		HALT();
	}
	ret = (char)serial->rx_char;
	serial->rx_char = NO_CHAR;
	return ret;
}


void
serial_putc(serial_t *serial, const char value) {
	USART_TypeDef *u = serial->device;
	CoEnterMutexSection(serial->mutex_id);
	//u->CR1 |= USART_CR1_TXEIE;
	while (!(u->SR & USART_SR_TXE)) {
		/* Interrupt handler will set flag when TXE is set */
		CoWaitForSingleFlag(serial->tx_flag, 1);
	}
	u->DR = value;
	CoLeaveMutexSection(serial->mutex_id);
}


void
serial_puts(serial_t *serial, const char *value) {
	USART_TypeDef *u = serial->device;
	CoEnterMutexSection(serial->mutex_id);
	while (*value) {
		//u->CR1 |= USART_CR1_TXEIE;
		while (!(u->SR & USART_SR_TXE)) {
			/* Interrupt handler will set flag when TXE is set */
			//CoWaitForSingleFlag(serial->tx_flag, 1);
		}
		u->DR = *value++;
	}
	CoLeaveMutexSection(serial->mutex_id);
}


static void
service_interrupt(serial_t *serial) {
	USART_TypeDef *u = serial->device;
	uint16_t cr1, sr, dr;
	cr1 = u->CR1;
	sr = u->SR;
	dr = u->DR;

	if (sr & USART_SR_RXNE) {
		serial->rx_char = dr;
		isr_SetFlag(serial->rx_flag);
	}
	if ((cr1 & USART_CR1_TXEIE) && (sr & USART_SR_TXE)) {
		u->CR1 &= ~USART_CR1_TXEIE;
		isr_SetFlag(serial->tx_flag);
	}
}


void
USART1_IRQHandler(void) {
	CoEnterISR();
	service_interrupt(&Serial1);
	CoExitISR();
}


void
UART4_IRQHandler(void) {
	CoEnterISR();
	service_interrupt(&Serial4);
	CoExitISR();
}
