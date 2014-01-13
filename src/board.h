/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _BOARD_H_
#define _BOARD_H_


#define BOARD_NAME				"Laureline NTP Server " BOARD_REV
#define BOARD_REV				"rev. 5"
#define STM32F10X_CL

/*
 * Ethernet PHY type.
 */
#define BOARD_PHY_ID            0x00221556
#define BOARD_PHY_ADDRESS		0
#define BOARD_PHY_RMII


#define LED1_PAD			GPIOC
#define LED1_PIN			(1<<0)
#define LED2_PAD			GPIOC
#define LED2_PIN			(1<<3)
#define LED3_PAD			GPIOC
#define LED3_PIN			(1<<2)
#define LED4_PAD			GPIOA
#define LED4_PIN			(1<<0)

#define PPSEN_PAD			GPIOC
#define PPSEN_PIN			(1<<9)

#define ETH_LED_PAD			GPIOB
#define ETH_LED_PIN			(1<<14)
#define E_NRST_PAD			GPIOA
#define E_NRST_PIN			(1<<4)
#define PPS_PAD				GPIOC
#define PPS_PNUM			6
#define PPS_PIN				(1<<PPS_PNUM)
#define SDIO_CS_PAD			GPIOB
#define SDIO_CS_PNUM		2
#define SDIO_PDOWN_PAD		GPIOA
#define SDIO_PDOWN_PIN		(1<<6)

#define DEFAULT_BAUD		57600
#define I2C_FREQ			100000

#define FEAT_MIN_PPSEN		0x0700
#define FEAT_MAX_PPSEN		0x07FF
#define HAS_FEATURE(x) ((hwver >= _PASTE2(FEAT_MIN_, x)) && \
		(hwver <= _PASTE2(FEAT_MAX_, x)))

extern uint16_t hwver;

void unstick_i2c(void);

#endif /* _BOARD_H_ */
