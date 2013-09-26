/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "init.h"
#include "periph/spi.h"

#if USE_SPI1
spi_t SPI1_Dev;
#endif
#if USE_SPI3
spi_t SPI3_Dev;
#endif

static void rx_isr(void *param, uint32_t flags);


void
spi_start(spi_t *spi, uint32_t cr1) {
	ASSERT(spi->cs_pad != NULL);
	spi->sem = CoCreateSem(0, 1, EVENT_SORT_TYPE_FIFO);
	ASSERT(spi->sem != E_CREATE_FAIL);
#if USE_SPI1
	if (spi == &SPI1_Dev) {
		spi->spi = SPI1;
		spi->tx_dma = &dma_streams[2];
		spi->rx_dma = &dma_streams[1];
	} else
#endif
#if USE_SPI3
	if (spi == &SPI3_Dev) {
		spi->spi = SPI3;
		spi->tx_dma = &dma_streams[8];
		spi->rx_dma = &dma_streams[7];
	} else
#endif
	{
		HALT();
	}
	dma_allocate(spi->tx_dma, IRQ_PRIO_SPI, NULL, NULL);
	dma_allocate(spi->rx_dma, IRQ_PRIO_SPI, rx_isr, spi);
	spi->tx_dma->ch->CPAR = (uint32_t)&spi->spi->DR;
	spi->rx_dma->ch->CPAR = (uint32_t)&spi->spi->DR;
	spi->tx_dma_mode = DMA_CCR1_DIR;
	spi->rx_dma_mode = DMA_CCR1_TCIE;
	if (cr1 & SPI_CR1_DFF) {
		/* 16 bit mode */
		spi->tx_dma_mode |= DMA_CCR1_PSIZE_0 | DMA_CCR1_MSIZE_0;
		spi->rx_dma_mode |= DMA_CCR1_PSIZE_0 | DMA_CCR1_MSIZE_0;
	}
	spi->spi->CR1 = 0;
	spi->spi->CR1 = cr1
		| SPI_CR1_MSTR
		| SPI_CR1_SSM
		| SPI_CR1_SSI
		;
	spi->spi->CR2 = 0
		| SPI_CR2_RXDMAEN
		| SPI_CR2_TXDMAEN
		| SPI_CR2_SSOE
		;
	spi->spi->CR1 |= SPI_CR1_SPE;
}

static uint32_t tx_dummy;
static uint32_t rx_dummy;

void
spi_exchange(spi_t *spi, const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t size) {
	dma_disable(spi->tx_dma);
	dma_disable(spi->rx_dma);
	if (tx_buf != NULL) {
		spi->tx_dma->ch->CCR = spi->tx_dma_mode | DMA_CCR1_MINC;
		spi->tx_dma->ch->CMAR = (uint32_t)tx_buf;
	} else {
		spi->tx_dma->ch->CCR = spi->tx_dma_mode;
		spi->tx_dma->ch->CMAR = (uint32_t)&tx_dummy;
		tx_dummy = 0xFF;
	}
	if (rx_buf != NULL) {
		spi->rx_dma->ch->CCR = spi->rx_dma_mode | DMA_CCR1_MINC;
		spi->rx_dma->ch->CMAR = (uint32_t)rx_buf;
	} else {
		spi->rx_dma->ch->CCR = spi->rx_dma_mode;
		spi->rx_dma->ch->CMAR = (uint32_t)&rx_dummy;
	}
	spi->tx_dma->ch->CNDTR = size;
	spi->rx_dma->ch->CNDTR = size;
	dma_enable(spi->tx_dma);
	dma_enable(spi->rx_dma);
	CoPendSem(spi->sem, 0);
}


static void
rx_isr(void *param, uint32_t flags) {
	spi_t *spi = (spi_t*)param;
	dma_disable(spi->tx_dma);
	dma_disable(spi->rx_dma);
	isr_PostSem(spi->sem);
}
