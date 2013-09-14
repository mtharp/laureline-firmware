/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "eth_mac.h"
#include "mii.h"

#define RX_BUFS 4
#define TX_BUFS 2
#define BUF_SIZE 1522
#define BUF_WORDS ((((BUF_SIZE - 1) | 3) + 1) / 4)


static volatile uint32_t rx_descs[RX_BUFS][4];
static volatile uint32_t tx_descs[TX_BUFS][4];
static uint32_t rx_bufs[RX_BUFS][BUF_WORDS];
static uint32_t tx_bufs[TX_BUFS][BUF_WORDS];


void
smi_write(uint32_t reg, uint32_t value) {
	ETH->MACMIIDR = value;
	ETH->MACMIIAR = 0
		| BOARD_PHY_ADDRESS
		| (reg << 6)
		| ETH_MACMIIAR_CR_Div42
		| ETH_MACMIIAR_MW
		| ETH_MACMIIAR_MB
		;
	while (ETH->MACMIIAR & ETH_MACMIIAR_MB) {}
}


uint32_t
smi_read(uint32_t reg) {
	ETH->MACMIIAR = 0
		| BOARD_PHY_ADDRESS
		| (reg << 6)
		| ETH_MACMIIAR_CR_Div42
		| ETH_MACMIIAR_MB
		;
	while (ETH->MACMIIAR & ETH_MACMIIAR_MB) {}
	return ETH->MACMIIDR;
}


uint8_t
smi_poll_link_status(void) {
	uint32_t maccr, bmsr, bmcr, lpa;
	maccr = ETH->MACCR;

	(void)smi_read(MII_BMSR);
	bmsr = smi_read(MII_BMSR);
	bmcr = smi_read(MII_BMCR);

	if (bmcr & BMCR_ANENABLE) {
		if ((bmsr & (BMSR_LSTATUS | BMSR_RFAULT | BMSR_ANEGCOMPLETE))
				!= (BMSR_LSTATUS | BMSR_ANEGCOMPLETE)) {
			return 0;
		}
		lpa = smi_read(MII_LPA);
		if (lpa & (LPA_100HALF | LPA_100FULL | LPA_100BASE4)) {
			maccr |= ETH_MACCR_FES;
		} else {
			maccr &= ~ETH_MACCR_FES;
		}
		if (lpa & (LPA_10FULL | LPA_100FULL)) {
			maccr |= ETH_MACCR_DM;
		} else {
			maccr &= ~ETH_MACCR_DM;
		}
	} else {
		if (!(bmsr & BMSR_LSTATUS)) {
			return 0;
		}
		if (bmcr & BMCR_SPEED100) {
			maccr |= ETH_MACCR_FES;
		} else {
			maccr &= ~ETH_MACCR_FES;
		}
		if (bmcr & BMCR_FULLDPLX) {
			maccr |= ETH_MACCR_DM;
		} else {
			maccr &= ~ETH_MACCR_DM;
		}
	}
	ETH->MACCR = maccr;
	return 1;
}


void
mac_set_hwaddr(const uint8_t *hwaddr) {
	ETH->MACA0HR = ((uint32_t)hwaddr[5] <<  8)
				|  ((uint32_t)hwaddr[4] <<  0);
	ETH->MACA0LR = ((uint32_t)hwaddr[3] << 24)
				|  ((uint32_t)hwaddr[2] << 16)
				|  ((uint32_t)hwaddr[1] <<  8)
				|  ((uint32_t)hwaddr[0] <<  0);
}


void
mac_start(void) {
	int i;
	RCC->AHBRSTR |=  RCC_AHBRSTR_ETHMACRST;
	RCC->AHBRSTR &= ~RCC_AHBRSTR_ETHMACRST;
	RCC->AHBENR |= RCC_AHBENR_ETHMACEN;

	/* MAC configuration */
	ETH->MACFFR = 0;
	ETH->MACFCR = 0;
	ETH->MACVLANTR = 0;

	ETH->MACA0HR = 0x0000FFFF;
	ETH->MACA0LR = 0xFFFFFFFF;
	ETH->MACA1HR = 0x0000FFFF;
	ETH->MACA1LR = 0xFFFFFFFF;
	ETH->MACA2HR = 0x0000FFFF;
	ETH->MACA2LR = 0xFFFFFFFF;
	ETH->MACA3HR = 0x0000FFFF;
	ETH->MACA3LR = 0xFFFFFFFF;
	ETH->MACHTHR = 0;
	ETH->MACHTLR = 0;

	ETH->MACCR = 0
		| ETH_MACCR_IPCO
		| ETH_MACCR_RE
		| ETH_MACCR_TE
		;

	/* Reset PHY */
	smi_write(MII_BMCR, BMCR_RESET);
	while (smi_read(MII_BMCR) & BMCR_RESET) {}

	/* Configure DMA */
	for (i = 0; i < RX_BUFS; i++) {
		rx_descs[i][0] = STM32_RDES0_OWN;
		rx_descs[i][1] = STM32_RDES1_RCH | BUF_SIZE;
		rx_descs[i][2] = (uint32_t)rx_bufs[i];
		rx_descs[i][3] = (uint32_t)&rx_descs[(i+1) % RX_BUFS];
	}
	for (i = 0; i < TX_BUFS; i++) {
		tx_descs[i][0] = STM32_TDES0_TCH;
		tx_descs[i][1] = 0;
		tx_descs[i][2] = (uint32_t)tx_bufs[i];
		tx_descs[i][3] = (uint32_t)&tx_descs[(i+1) % TX_BUFS];
	}
	ETH->DMABMR |= ETH_DMABMR_SR;
	while (ETH->DMABMR & ETH_DMABMR_SR) {}
	ETH->DMARDLAR = (uint32_t)rx_descs;
	ETH->DMATDLAR = (uint32_t)tx_descs;
	ETH->DMASR = ETH->DMASR;
	ETH->DMAIER = 0
		| ETH_DMAIER_NISE
		| ETH_DMAIER_RIE
		| ETH_DMAIER_TIE
		;
	ETH->DMABMR = 0
		| ETH_DMABMR_AAB
		| ETH_DMABMR_RDP_1Beat
		| ETH_DMABMR_PBL_1Beat
		;
	ETH->DMAOMR = ETH_DMAOMR_FTF;
	while(ETH->DMAOMR & ETH_DMAOMR_FTF) {}
	ETH->DMAOMR = 0
		| ETH_DMAOMR_DTCEFD
		| ETH_DMAOMR_RSF
		| ETH_DMAOMR_TSF
		| ETH_DMAOMR_ST
		| ETH_DMAOMR_SR
		;
}
