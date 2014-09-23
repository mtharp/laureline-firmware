/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "init.h"
#include "stm32/i2c.h"


static void i2c_configure(i2c_t *i2c);

#if USE_I2C1
i2c_t I2C1_Dev = {I2C1, I2C_T_INITIALIZER};
#endif
#if USE_I2C2
i2c_t I2C2_Dev = {I2C2, I2C_T_INITIALIZER};
#endif

#define FENCE() asm volatile("dmb" ::: "memory")


#if USE_I2C1 || USE_I2C2
static void
handle_i2c_event(i2c_t *i2c) {
    I2C_TypeDef *d = i2c->dev;
    uint16_t sr1 = d->SR1;
    uint16_t timeout;
    if (sr1 & I2C_SR1_SB) {
        /* Start bit is sent, now send address */
        d->CR1 |= I2C_CR1_ACK;
        i2c->index = 0;
        if ((i2c->addr_dir & 1) && i2c->count == 2) {
            /* Give advance notice of NACK after a 2-byte read */
            d->CR1 |= I2C_CR1_POS;
        }
        d->DR = i2c->addr_dir;

    } else if (sr1 & I2C_SR1_ADDR) {
        /* Address is sent, now send data */
        FENCE();
        if ((i2c->addr_dir & 1) && i2c->count == 1) {
            /* Receiving 1 byte */
            d->CR1 &= ~I2C_CR1_ACK;
            FENCE();
            (void)d->SR2; /* clear ADDR */
            d->CR1 |= I2C_CR1_STOP;
            d->CR2 |= I2C_CR2_ITBUFEN;
        } else {
            (void)d->SR2; /* clear ADDR */
            FENCE();
            if ((i2c->addr_dir & 1) && i2c->count == 2) {
                /* Receiving 2 bytes */
                d->CR1 &= ~I2C_CR1_ACK;
                d->CR2 &= ~I2C_CR2_ITBUFEN;
            } else if ((i2c->addr_dir & 1) && i2c->count == 3) {
                /* Receiving 3 bytes */
                d->CR2 &= ~I2C_CR2_ITBUFEN;
            } else {
                /* Receiving 4+ bytes, or transmitting */
                d->CR2 |= I2C_CR2_ITBUFEN;
            }
        }

    } else if (sr1 & I2C_SR1_BTF) {
        /* Byte transfer finished */
        if (i2c->addr_dir & 1) {
            if (i2c->count > 2) {
                /* Normal receive */
                d->CR1 &= ~I2C_CR1_ACK;
                i2c->buf[i2c->index++] = d->DR;
                d->CR1 |= I2C_CR1_STOP;
                i2c->buf[i2c->index++] = d->DR;
                d->CR2 |= I2C_CR2_ITBUFEN;
            } else {
                /* Short receive */
                d->CR1 |= I2C_CR1_STOP;
                i2c->buf[i2c->index++] = d->DR;
                i2c->buf[i2c->index++] = d->DR;
                i2c->index++; /* done */
            }
        } else {
            /* Transmit complete */
            d->CR1 |= I2C_CR1_STOP;
            i2c->index++; /* done */
        }

    } else if (sr1 & I2C_SR1_RXNE) {
        i2c->buf[i2c->index++] = d->DR;
        if (i2c->index + 3 == i2c->count) {
            /* Disable TXE to allow the buffer to flush */
            d->CR2 &= ~I2C_CR2_ITBUFEN;
        } else if (i2c->index == i2c->count) {
            i2c->index++; /* done */
        }

    } else if (sr1 & I2C_SR1_TXE) {
        /* Byte transmitted */
        d->DR = i2c->buf[i2c->index++];
        if (i2c->index == i2c->count) {
            /* Disable TXE to allow the buffer to flush */
            d->CR2 &= ~I2C_CR2_ITBUFEN;
        }
    }

    if (i2c->index == i2c->count + 1) {
        /* Done, disable interrupts until next transaction */
        d->CR1 &= ~I2C_CR1_POS;
        d->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITERREN | I2C_CR2_ITBUFEN);
        timeout = 15000;
        while (d->CR1 & (I2C_CR1_START | I2C_CR1_STOP)) {
            if (--timeout == 0) {
                i2c->error = EERR_TIMEOUT;
            }
        }
        /* Wake up userspace */
        isr_PostSem(i2c->sem);
    }
}


static void
handle_i2c_error(i2c_t *i2c) {
    I2C_TypeDef *d = i2c->dev;
    uint16_t sr1 = d->SR1;
    (void)d->SR2;
    if (sr1 & I2C_SR1_OVR) {
        i2c->error = EERR_FAULT;
    } else if (sr1 & I2C_SR1_ARLO) {
        i2c->error = EERR_AGAIN;
        d->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITERREN | I2C_CR2_ITBUFEN);
    } else if (sr1 & (I2C_SR1_AF | I2C_SR1_BERR)) {
        if (sr1 & I2C_SR1_AF) {
            i2c->error = EERR_NACK;
        } else {
            i2c->error = EERR_FAULT;
        }
        d->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITERREN | I2C_CR2_ITBUFEN);
        if (d->CR1 & I2C_CR1_START) {
            /* START+STOP hangs the peripheral, so reset afterwards */
            while (d->CR1 & I2C_CR1_START) {}
            d->CR1 |= I2C_CR1_STOP;
            while (d->CR1 & I2C_CR1_STOP) {}
            i2c_configure(i2c);
        } else {
            d->CR1 |= I2C_CR1_STOP;
        }
    } else {
        /* No error. Why are we here? */
        return;
    }
    /* Clear errors and wake up userspace */
    isr_PostSem(i2c->sem);
    d->SR1 &= ~0x0F00;
}
#endif


#if USE_I2C1
void
I2C1_EV_IRQHandler(void) {
    CoEnterISR();
    handle_i2c_event(&I2C1_Dev);
    CoExitISR();
}


void
I2C1_ER_IRQHandler(void) {
    CoEnterISR();
    handle_i2c_error(&I2C1_Dev);
    CoExitISR();
}
#endif


#if USE_I2C2
void
I2C2_EV_IRQHandler(void) {
    CoEnterISR();
    handle_i2c_event(&I2C2_Dev);
    CoExitISR();
}


void
I2C2_ER_IRQHandler(void) {
    CoEnterISR();
    handle_i2c_error(&I2C2_Dev);
    CoExitISR();
}
#endif


void
i2c_start(i2c_t *i2c) {
    if (i2c->mutex == E_CREATE_FAIL) {
        i2c->mutex = CoCreateMutex();
        if (i2c->mutex == E_CREATE_FAIL) { HALT(); }
    }
    if (i2c->sem == E_CREATE_FAIL) {
        i2c->sem = CoCreateSem(0, 1, EVENT_SORT_TYPE_FIFO);
        if (i2c->sem == E_CREATE_FAIL) { HALT(); }
    }
    CoEnterMutexSection(i2c->mutex);
    i2c_configure(i2c);
}


static void
i2c_configure(i2c_t *i2c) {
    I2C_TypeDef *d = i2c->dev;
#if USE_I2C1
    if (i2c == &I2C1_Dev) {
        RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
        RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
        RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;
        NVIC_SetPriority(I2C1_EV_IRQn, IRQ_PRIO_I2C);
        NVIC_SetPriority(I2C1_ER_IRQn, IRQ_PRIO_I2C);
        NVIC_EnableIRQ(I2C1_EV_IRQn);
        NVIC_EnableIRQ(I2C1_ER_IRQn);
    } else
#endif
#if USE_I2C2
    if (i2c == &I2C2_Dev) {
        RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;
        RCC->APB1RSTR |= RCC_APB1RSTR_I2C2RST;
        RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C2RST;
        NVIC_SetPriority(I2C2_EV_IRQn, IRQ_PRIO_I2C);
        NVIC_SetPriority(I2C2_ER_IRQn, IRQ_PRIO_I2C);
        NVIC_EnableIRQ(I2C2_EV_IRQn);
        NVIC_EnableIRQ(I2C2_ER_IRQn);
    } else
#endif
    {
        HALT();
    }
    d->CR1 = I2C_CR1_SWRST;
    d->CR1 = 0;
    /* FIXME: assuming APB1 = sysclk / 2 */
    d->CR2 = (uint16_t)(system_frequency/2 / 1e6);
    d->CCR = (uint16_t)((system_frequency/2) / 2 / 10e3);
    d->TRISE = (uint16_t)(1e-6 / (system_frequency/2) + 1);
    d->CR1 = I2C_CR1_PE;
}


void
i2c_stop(i2c_t *i2c) {
    i2c->dev->CR1 = 0;
    CoLeaveMutexSection(i2c->mutex);
}


int16_t
i2c_transact(i2c_t *i2c, uint8_t addr_dir,
        uint8_t *buf, size_t count) {
    i2c->addr_dir = addr_dir;
    i2c->buf = buf;
    i2c->count = count;
    i2c->error = 0;
    /* Begin the transaction */
    i2c->dev->CR1 |= I2C_CR1_START;
    i2c->dev->CR2 |= I2C_CR2_ITEVTEN | I2C_CR2_ITERREN;
    /* Wait for magic to happen */
    CoPendSem(i2c->sem, 0);
    /* Check if it worked */
    return i2c->error;
}
