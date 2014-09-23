/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "stm32/dma.h"

const dma_ch_t dma_streams[DMA_STREAMS] = {
    {DMA1_Channel1, DMA1_Channel1_IRQn,  0, &DMA1->IFCR,  0},
    {DMA1_Channel2, DMA1_Channel2_IRQn,  1, &DMA1->IFCR,  4},
    {DMA1_Channel3, DMA1_Channel3_IRQn,  2, &DMA1->IFCR,  8},
    {DMA1_Channel4, DMA1_Channel4_IRQn,  3, &DMA1->IFCR, 12},
    {DMA1_Channel5, DMA1_Channel5_IRQn,  4, &DMA1->IFCR, 16},
    {DMA1_Channel6, DMA1_Channel6_IRQn,  5, &DMA1->IFCR, 20},
    {DMA1_Channel7, DMA1_Channel7_IRQn,  6, &DMA1->IFCR, 24},

    {DMA2_Channel1, DMA2_Channel1_IRQn,  7, &DMA2->IFCR,  0},
    {DMA2_Channel2, DMA2_Channel2_IRQn,  8, &DMA2->IFCR,  4},
    {DMA2_Channel3, DMA2_Channel3_IRQn,  9, &DMA2->IFCR,  8},
    {DMA2_Channel4, DMA2_Channel4_IRQn, 10, &DMA2->IFCR, 12},
    {DMA2_Channel5, DMA2_Channel5_IRQn, 11, &DMA2->IFCR, 16},
};

static dma_isr_t isr_funcs[DMA_STREAMS];
static void *isr_params[DMA_STREAMS];


void
dma_allocate(const dma_ch_t *ch, uint32_t irq_priority, dma_isr_t func, void *param) {
    isr_funcs[ch->index] = func;
    isr_params[ch->index] = param;
    dma_disable(ch);
    ch->ch->CCR = 0;
    NVIC_SetPriority(ch->vector, irq_priority);
    NVIC_EnableIRQ(ch->vector);
}


void
dma_release(const dma_ch_t *ch) {
    NVIC_DisableIRQ(ch->vector);
    dma_disable(ch);
}


static void
dma_service_irq(DMA_TypeDef *dma, const dma_ch_t *ch) {
    uint32_t flags;
    flags = (dma->ISR >> ch->ishift) & 0xF;
    dma->IFCR = 0xF << ch->ishift;
    if (isr_funcs[ch->index]) {
        isr_funcs[ch->index](isr_params[ch->index], flags);
    }
}


void
DMA1_Channel1_IRQHandler(void) {
    CoEnterISR();
    dma_service_irq(DMA1, &dma_streams[0]);
    CoExitISR();
}


void
DMA1_Channel2_IRQHandler(void) {
    CoEnterISR();
    dma_service_irq(DMA1, &dma_streams[1]);
    CoExitISR();
}


void
DMA1_Channel3_IRQHandler(void) {
    CoEnterISR();
    dma_service_irq(DMA1, &dma_streams[2]);
    CoExitISR();
}


void
DMA1_Channel4_IRQHandler(void) {
    CoEnterISR();
    dma_service_irq(DMA1, &dma_streams[3]);
    CoExitISR();
}


void
DMA1_Channel5_IRQHandler(void) {
    CoEnterISR();
    dma_service_irq(DMA1, &dma_streams[4]);
    CoExitISR();
}


void
DMA1_Channel6_IRQHandler(void) {
    CoEnterISR();
    dma_service_irq(DMA1, &dma_streams[5]);
    CoExitISR();
}


void
DMA1_Channel7_IRQHandler(void) {
    CoEnterISR();
    dma_service_irq(DMA1, &dma_streams[6]);
    CoExitISR();
}


void
DMA2_Channel1_IRQHandler(void) {
    CoEnterISR();
    dma_service_irq(DMA1, &dma_streams[7]);
    CoExitISR();
}


void
DMA2_Channel2_IRQHandler(void) {
    CoEnterISR();
    dma_service_irq(DMA1, &dma_streams[8]);
    CoExitISR();
}


void
DMA2_Channel3_IRQHandler(void) {
    CoEnterISR();
    dma_service_irq(DMA1, &dma_streams[9]);
    CoExitISR();
}


void
DMA2_Channel4_IRQHandler(void) {
    CoEnterISR();
    dma_service_irq(DMA1, &dma_streams[10]);
    CoExitISR();
}


void
DMA2_Channel5_IRQHandler(void) {
    CoEnterISR();
    dma_service_irq(DMA1, &dma_streams[11]);
    CoExitISR();
}
