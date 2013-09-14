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


static void
mii_write(uint32_t reg, uint32_t value) {
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


static uint32_t
mii_read(uint32_t reg) {
	ETH->MACMIIAR = 0
		| BOARD_PHY_ADDRESS
		| (reg << 6)
		| ETH_MACMIIAR_CR_Div42
		| ETH_MACMIIAR_MB
		;
	while (ETH->MACMIIAR & ETH_MACMIIAR_MB) {}
	return ETH->MACMIIDR;
}


void
mac_start(void) {
	RCC->AHBRSTR |=  RCC_AHBRSTR_ETHMACRST;
	RCC->AHBRSTR &= ~RCC_AHBRSTR_ETHMACRST;
	RCC->AHBENR |= RCC_AHBENR_ETHMACEN;

	/* Reset PHY */
	mii_write(MII_BMCR, BMCR_RESET);
	while (mii_read(MII_BMCR) & BMCR_RESET) {}
}
