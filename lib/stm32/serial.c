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


static void usart_tcie(void *param, uint32_t flags);


void
serial_start(serial_t *serial, int speed
#if configUSE_QUEUE_SETS
        , QueueSetHandle_t queue_set
#endif
        ) {
    IRQn_Type irqn = 0;
    ASSERT((serial->rx_q = xQueueCreate(SERIAL_RX_SIZE, 1)));
    ASSERT((serial->mutex = xSemaphoreCreateMutex()));
#if configUSE_QUEUE_SETS
    if (queue_set) {
        /* Must be added to set while it's still empty */
        xQueueAddToSet(serial->rx_q, queue_set);
    }
#endif
    serial->speed = speed;
    serial->tx_dma = NULL;
#ifdef USE_SERIAL_USART1
    if (serial == &Serial1) {
        RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
        irqn = USART1_IRQn;
        serial->usart = USART1;
        serial->tx_dma = &dma_streams[3];
    } else
#endif
#if USE_SERIAL_UART4
    if (serial == &Serial4) {
        RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
        irqn = UART4_IRQn;
        serial->usart = UART4;
        serial->tx_dma = &dma_streams[11];
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
    serial->usart->CR3 = 0;
    if (serial->tx_dma) {
        dma_allocate(serial->tx_dma, IRQ_PRIO_USART, usart_tcie, serial);
        serial->tx_dma->ch->CPAR = (uint32_t)&serial->usart->DR;
        serial->usart->CR3 |= USART_CR3_DMAT;
        serial->tx_q = NULL;
        ASSERT((serial->tcie_sem = xSemaphoreCreateBinary()));
    } else {
        ASSERT((serial->tx_q = xQueueCreate(SERIAL_TX_SIZE, 1)));
        serial->tcie_sem = NULL;
    }
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


static void
_serial_write(serial_t *serial, const char *value, uint16_t size) {
    serial->usart->SR = ~USART_SR_TC;
    if (serial->tx_dma) {
        dma_disable(serial->tx_dma);
        serial->tx_dma->ch->CCR = 0
            | DMA_CCR1_DIR
            | DMA_CCR1_MINC
            | DMA_CCR1_TEIE
            | DMA_CCR1_TCIE
            ;
        serial->tx_dma->ch->CMAR = (uint32_t)value;
        serial->tx_dma->ch->CNDTR = size;
        dma_enable(serial->tx_dma);
        xSemaphoreTake(serial->tcie_sem, portMAX_DELAY);
    } else {
        while (size--) {
            _serial_putc(serial, value, portMAX_DELAY);
            value++;
        }
    }
}



void
serial_puts(serial_t *serial, const char *value) {
    xSemaphoreTake(serial->mutex, portMAX_DELAY);
    _serial_write(serial, value, strlen(value));
    xSemaphoreGive(serial->mutex);
}


void
serial_write(serial_t *serial, const char *value, uint16_t size) {
    xSemaphoreTake(serial->mutex, portMAX_DELAY);
    _serial_write(serial, value, size);
    xSemaphoreGive(serial->mutex);
}


void
serial_printf(serial_t *serial, const char *fmt, ...) {
    unsigned len;
    static char fmt_buf[64];
    va_list ap;
    va_start(ap, fmt);
    xSemaphoreTake(serial->mutex, portMAX_DELAY);
    len = vsnprintf(fmt_buf, sizeof(fmt_buf), fmt, ap);
    _serial_write(serial, fmt_buf, MIN(len, sizeof(fmt_buf)));
    va_end(ap);
    xSemaphoreGive(serial->mutex);
}


void
serial_drain(serial_t *serial) {
    xSemaphoreTake(serial->mutex, portMAX_DELAY);
    while (!(serial->usart->SR & USART_SR_TC)) {}
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
usart_irq(serial_t *serial) {
    USART_TypeDef *u = serial->usart;
    uint16_t sr, dr;
    uint8_t val;
    BaseType_t wakeup = 0;
    sr = u->SR;
    dr = u->DR;

    if ((sr & USART_SR_RXNE) && serial->rx_q) {
        xQueueSendFromISR(serial->rx_q, &dr, &wakeup);
    }
    if (sr & USART_SR_TXE) {
        if (serial->tx_q && xQueueReceiveFromISR(serial->tx_q, &val, &wakeup)) {
            u->DR = val;
        } else {
            u->CR1 &= ~USART_CR1_TXEIE;
        }
    }

    portEND_SWITCHING_ISR(wakeup);
}


static void
usart_tcie(void *param, uint32_t flags) {
    serial_t *serial = (serial_t*)param;
    BaseType_t wakeup = 0;
    dma_disable(serial->tx_dma);
    xSemaphoreGiveFromISR(serial->tcie_sem, &wakeup);
    portEND_SWITCHING_ISR(wakeup);
}


#if USE_SERIAL_USART1
void
USART1_IRQHandler(void) {
    usart_irq(&Serial1);
}
#endif


#if USE_SERIAL_UART4
void
UART4_IRQHandler(void) {
    usart_irq(&Serial4);
}
#endif


#if USE_SERIAL_UART5
void
UART5_IRQHandler(void) {
    usart_irq(&Serial5);
}
#endif
