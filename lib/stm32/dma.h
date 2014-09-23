/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _DMA_H
#define _DMA_H

#include "common.h"


typedef struct {
    DMA_Channel_TypeDef *ch;
    IRQn_Type vector;
    uint8_t index;
    volatile uint32_t *ifcr;
    uint8_t ishift;
} dma_ch_t;

typedef void (*dma_isr_t)(void *param, uint32_t flags);


#define DMA_STREAMS     12
extern const dma_ch_t dma_streams[DMA_STREAMS];

void dma_allocate(const dma_ch_t *ch, uint32_t irq_priority, dma_isr_t func, void *param);
void dma_release(const dma_ch_t *ch);
#define dma_enable(chn) { (chn)->ch->CCR |= DMA_CCR1_EN; }
#define dma_disable(chn) { \
    (chn)->ch->CCR &= ~DMA_CCR1_EN; \
    *(chn)->ifcr = 0xF << (chn)->ishift; \
}


#endif
