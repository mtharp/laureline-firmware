/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */


#include "common.h"
#include "init.h"
#include "stm32/serial.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#if USE_SERIAL_USART1
serial_t Serial1;
#endif
#if USE_SERIAL_UART4
serial_t Serial4;
#endif
#if USE_SERIAL_UART5
serial_t Serial5;
#endif

#define _serial_putc(serial, val_ptr, timeout) \
    xQueueSend(serial->tx_q, val_ptr, timeout); \
    serial->usart->CR1 |= USART_CR1_TXEIE;


void
serial_start(serial_t *serial, int speed, QueueSetHandle_t queue_set) {
    IRQn_Type irqn = 0;
    serial->tx_q = xQueueCreate(SERIAL_TX_SIZE, 1);
    serial->rx_q = xQueueCreate(SERIAL_RX_SIZE, 1);
    serial->mutex = xSemaphoreCreateMutex();
    ASSERT(serial->tx_q && serial->rx_q && serial->mutex);
    if (queue_set) {
        /* Must be added to set while it's still empty */
        xQueueAddToSet(serial->rx_q, queue_set);
    }
    serial->speed = speed;
#ifdef USE_SERIAL_USART1
    if (serial == &Serial1) {
        RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
        irqn = USART1_IRQn;
        serial->usart = USART1;
    } else
#endif
#if 0
    if (serial == &Serial2) {
        RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
        irqn = USART2_IRQn;
        serial->usart = USART2;
    } else
#endif
#if 0
    if (serial == &Serial3) {
        RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
        irqn = USART3_IRQn;
        serial->usart = USART3;
    } else
#endif
#if USE_SERIAL_UART4
    if (serial == &Serial4) {
        RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
        irqn = UART4_IRQn;
        serial->usart = UART4;
    } else
#endif
#if USE_SERIAL_UART5
    if (serial == &Serial5) {
        RCC->APB1ENR |= RCC_APB1ENR_UART5EN;
        irqn = UART5_IRQn;
        serial->usart = UART5;
    } else
#endif
    {
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
        ;
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
    xSemaphoreTake(serial->mutex, portMAX_DELAY);
    while (*value) {
        _serial_putc(serial, value, portMAX_DELAY);
        value++;
    }
    xSemaphoreGive(serial->mutex);
}


void
serial_write(serial_t *serial, const char *value, uint16_t size) {
    unsigned i;
    xSemaphoreTake(serial->mutex, portMAX_DELAY);
    for (i = 0; i < size; i++) {
        _serial_putc(serial, value, portMAX_DELAY);
        value++;
    }
    xSemaphoreGive(serial->mutex);
}


void
serial_printf(serial_t *serial, const char *fmt, ...) {
    unsigned i, len;
    static char fmt_buf[64];
    va_list ap;
    va_start(ap, fmt);
    xSemaphoreTake(serial->mutex, portMAX_DELAY);
    len = vsnprintf(fmt_buf, sizeof(fmt_buf), fmt, ap);
    for (i = 0; i < MIN(len, sizeof(fmt_buf)); i++) {
        _serial_putc(serial, &fmt_buf[i], portMAX_DELAY);
    }
    va_end(ap);
    xSemaphoreGive(serial->mutex);
}


int16_t
serial_get(serial_t *serial, TickType_t timeout) {
    uint8_t val;
    if (xQueueReceive(serial->rx_q, &val, timeout)) {
        return val;
    } else {
        return EERR_TIMEOUT;
    }
}


static void
service_interrupt(serial_t *serial) {
    USART_TypeDef *u = serial->usart;
    uint16_t sr, dr;
    uint8_t val;
    BaseType_t wakeup = 0;
    sr = u->SR;
    dr = u->DR;

    if (sr & USART_SR_RXNE) {
        xQueueSendFromISR(serial->rx_q, &dr, &wakeup);
    }
    if (sr & USART_SR_TXE) {
        if (xQueueReceiveFromISR(serial->tx_q, &val, &wakeup)) {
            u->DR = val;
        } else {
            u->CR1 &= ~USART_CR1_TXEIE;
        }
    }

    portEND_SWITCHING_ISR(wakeup);
}


#if USE_SERIAL_USART1
void
USART1_IRQHandler(void) {
    service_interrupt(&Serial1);
}
#endif


#if USE_SERIAL_UART4
void
UART4_IRQHandler(void) {
    service_interrupt(&Serial4);
}
#endif


#if USE_SERIAL_UART5
void
UART5_IRQHandler(void) {
    service_interrupt(&Serial5);
}
#endif
