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
#define ONBOARD_CLOCK			26000000
#define STM32F10X_CL

/*
 * Ethernet PHY type.
 */
#define BOARD_PHY_ID            0x00221556
#define BOARD_PHY_ADDRESS		0
#define BOARD_PHY_RMII

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 *
 * The digits have the following meaning:
 *   0 - Analog input.
 *   1 - Push Pull output 10MHz.
 *   2 - Push Pull output 2MHz.
 *   3 - Push Pull output 50MHz.
 *   4 - Digital input.
 *   5 - Open Drain output 10MHz.
 *   6 - Open Drain output 2MHz.
 *   7 - Open Drain output 50MHz.
 *   8 - Digital input with PullUp or PullDown resistor depending on ODR.
 *   9 - Alternate Push Pull output 10MHz.
 *   A - Alternate Push Pull output 2MHz.
 *   B - Alternate Push Pull output 50MHz.
 *   C - Reserved.
 *   D - Alternate Open Drain output 10MHz.
 *   E - Alternate Open Drain output 2MHz.
 *   F - Alternate Open Drain output 50MHz.
 * Please refer to the STM32 Reference Manual for details.
 */

/*
 * PA0	PP		LED4
 * PA1	IPU		ETH_RMII_REF_CLK
 * PA2	AF_OD	ETH_RMII_MDIO
 * PA3	IPU
 * PA4	PP		E_NRST
 * PA5	IPU
 * PA6	IPU
 * PA7	IPU		ETH_RMII_CRS_DV
 * PA8	IPU
 * PA9	AF_PP	USART1_TX
 * PA10	IPU		USART1_RX
 * PA11	IPU
 * PA12	IPU
 * PA13	IPU		SWDIO
 * PA14	IPU		SWCLK
 * PA15	IPU		ECLK TIM2_ETR
 */
#define VAL_GPIOACRL			0x88828F82		/*  7..0 */
#define VAL_GPIOACRH			0x888888A8		/* 15..8 */
#define VAL_GPIOAODR			0b1111111111111110

/*
 * PB0	IPU		SD_DAT1
 * PB1	IPU		SD_DAT2
 * PB2	PP		SD_CS
 * PB3	AF_PP	SD_SCK
 * PB4	IPU		SD_MISO
 * PB5	AF_PP	SD_MOSI
 * PB6	AF_OD	I2C1_SCL
 * PB7	AF_OD	I2C1_SDA
 * PB8	IPU
 * PB9	IPU
 * PB10	IPU
 * PB11	AF_PP	ETH_RMII_TX_EN
 * PB12	AF_PP	ETH_RMII_TXD0
 * PB13	AF_PP	ETH_RMII_TXD1
 * PB14	PP		E_LED
 * PB15	IPU
 */
#define VAL_GPIOBCRL			0xFFB8B288		/*  7..0 */
#define VAL_GPIOBCRH			0x82BBB888		/* 15..8 */
#define VAL_GPIOBODR			0b1011111111111111

/*
 * PC0	PP		LED1
 * PC1	AF_PP	ETH_MDC
 * PC2	PP		LED3
 * PC3	PP		LED2
 * PC4	IPU		ETH_RMII_RXD0
 * PC5	IPU		ETH_RMII_RXD1
 * PC6	IPD		PPS1 TIM3_IC1
 * PC7	IPU
 * PC8	IPD		PPS2 TIM3_IC3
 * PC9	IPU
 * PC10	AF_PP	UART4_TX
 * PC11	IPU		UART4_RX
 * PC12	AF_PP	UART5_TX
 * PC13	PP		CKSEL
 * PC14	IPU
 * PC15	IPU
 */
#define VAL_GPIOCCRL			0x888822B2		/*  7..0 */
#define VAL_GPIOCCRH			0x882A8A88		/* 15..8 */
#define VAL_GPIOCODR			0b1101111010110010

/*
 * PD0	IN		OSC_IN
 * PD1	IPU
 * PD2	IPU		UART5_RX
 */
#define VAL_GPIODCRL			0x88888888		/*  7..0 */
#define VAL_GPIODCRH			0x88888888		/* 15..8 */
#define VAL_GPIODODR			0b1111111111111111

#define VAL_GPIOECRL			0x88888888		/*  7..0 */
#define VAL_GPIOECRH			0x88888888		/* 15..8 */
#define VAL_GPIOEODR			0xFFFFFFFF


#define LED1_PAD			GPIOC
#define LED1_PIN			(1<<0)
#define LED2_PAD			GPIOC
#define LED2_PIN			(1<<3)
#define LED3_PAD			GPIOC
#define LED3_PIN			(1<<2)
#define LED4_PAD			GPIOA
#define LED4_PIN			(1<<0)

#define E_NRST_PAD			GPIOA
#define E_NRST_PIN			(1<<4)
#define CKSEL_PAD			GPIOC
#define CKSEL_PNUM			13
#define CKSEL_PIN			(1<<CKSEL_PNUM)
#define PPS_PAD				GPIOC
#define PPS_PNUM			6
#define PPS_PIN				(1<<PPS_PNUM)

#define BL_PAD				GPIOA
#define BL_PIN				(1<<10)
#define BL_SENSE			0

#define DEFAULT_BAUD		57600
#define I2C_FREQ			100000
#define HAS_SDIO			0

#endif /* _BOARD_H_ */
