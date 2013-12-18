/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "init.h"

static const gpio_cfg_t gpio_cfg[4][16] = {
	{
		{GPIO_MODE_2MHZ		| GPIO_OUTPUT_PP,	0}, /* PA0  - LED4 */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PA1  - ETH_RMII_REF_CLK */
		{GPIO_MODE_50MHZ	| GPIO_AFIO_OD,		1}, /* PA2  - ETH_RMII_MDIO */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PA3  - */
		{GPIO_MODE_2MHZ		| GPIO_OUTPUT_PP,	1}, /* PA4  - ETH_NRST*/
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PA5  - */
		{GPIO_MODE_2MHZ		| GPIO_OUTPUT_PP,	1}, /* PA6  - SD_PWR */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PA7  - ETH_RMII_CRS_DV */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PA8  - */
		{GPIO_MODE_2MHZ		| GPIO_AFIO_PP,		1}, /* PA9  - USART1_TX */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PA10 - USART1_RX */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PA11 - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PA12 - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PA13 - SWDIO */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PA14 - SWCLK */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}  /* PA15 - ECLK TIM2_ETR */
	}, {
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PB0  - SD_DAT1 */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PB1  - SD_DAT2 */
		{GPIO_MODE_2MHZ		| GPIO_OUTPUT_PP,	1}, /* PB2  - SD_CS */
		{GPIO_MODE_50MHZ	| GPIO_AFIO_PP,		1}, /* PB3  - SD_SCK */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PB4  - SD_MISO */
		{GPIO_MODE_50MHZ	| GPIO_AFIO_PP,		1}, /* PB5  - SD_MOSI */
		{GPIO_MODE_50MHZ	| GPIO_AFIO_OD,		1}, /* PB6  - I2C1_SCL */
		{GPIO_MODE_50MHZ	| GPIO_AFIO_OD,		1}, /* PB7  - I2C1_SDA */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PB8  - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PB9  - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PB10 - */
		{GPIO_MODE_50MHZ	| GPIO_AFIO_PP,		1}, /* PB11 - ETH_RMII_TX_EN */
		{GPIO_MODE_50MHZ	| GPIO_AFIO_PP,		1}, /* PB12 - ETH_RMII_TXD0 */
		{GPIO_MODE_50MHZ	| GPIO_AFIO_PP,		1}, /* PB13 - ETH_RMII_TXD1 */
		{GPIO_MODE_2MHZ		| GPIO_OUTPUT_PP,	1}, /* PB14 - ETH_LED */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}  /* PB15 - */
	}, {
		{GPIO_MODE_2MHZ		| GPIO_OUTPUT_PP,	0}, /* PC0  - LED1 */
		{GPIO_MODE_50MHZ	| GPIO_AFIO_PP,		1}, /* PC1  - ETH_RMII_MDC */
		{GPIO_MODE_2MHZ		| GPIO_OUTPUT_PP,	0}, /* PC2  - LED3 */
		{GPIO_MODE_2MHZ		| GPIO_OUTPUT_PP,	0}, /* PC3  - LED2 */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PC4  - ETH_RMII_RXD0 */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PC5  - ETH_RMII_RXD1 */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	0}, /* PC6  - PPS1 TIM3_IC1 */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PC7  - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	0}, /* PC8  - PPS1 TIM3_IC3 */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PC9  - */
		{GPIO_MODE_2MHZ		| GPIO_AFIO_PP,		1}, /* PC10 - UART4_TX */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PC11 - UART4_RX */
		{GPIO_MODE_2MHZ		| GPIO_AFIO_PP,		1}, /* PC12 - UART5_TX */
		{GPIO_MODE_2MHZ		| GPIO_OUTPUT_PP,	0}, /* PC13 - CKSEL */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PC14 - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}  /* PC15 - */
	}, {
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PD0  - OSC_IN */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PD1  - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PD2  - UART5_RX */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PD3  - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PD4  - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PD5  - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PD6  - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PD7  - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PD8  - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PD9  - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PD10 - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PD11 - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PD12 - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PD13 - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}, /* PD14 - */
		{GPIO_MODE_INPUT	| GPIO_INPUT_PUPD,	1}  /* PD15 - */
	}};


static void
delay10us(void) {
	int i;
	/* Assumes 8MHz HSI */
	for (i = 0; i < 27; i++) {
		__NOP();
	}
}


void
unstick_i2c(void) {
	int i;
	setup_gpio(GPIOB, 6, GPIO_MODE_2MHZ | GPIO_OUTPUT_OD, 1);
	setup_gpio(GPIOB, 7, GPIO_MODE_2MHZ | GPIO_OUTPUT_OD, 1);
	for (i = 0; i < 8; i++) {
		while (!(GPIOB->IDR & (1<<6))) { delay10us(); }
		GPIOB->BRR = (1<<6);
		delay10us();
		GPIOB->BSRR = (1<<6);
		delay10us();
	}
	GPIOB->BRR = (1<<7);
	delay10us();
	GPIOB->BRR = (1<<6);
	delay10us();
	GPIOB->BSRR = (1<<6);
	delay10us();
	GPIOB->BSRR = (1<<7);
	delay10us();
	setup_gpio(GPIOB, 6, GPIO_MODE_2MHZ | GPIO_AFIO_OD, 1);
	setup_gpio(GPIOB, 7, GPIO_MODE_2MHZ | GPIO_AFIO_OD, 1);
}


void
SystemInit(void) {
	/* Enable peripheral clocks */
	RCC->AHBENR |= 0
		| RCC_AHBENR_DMA1EN
		| RCC_AHBENR_DMA2EN
		| RCC_AHBENR_ETHMACEN
		| RCC_AHBENR_ETHMACTXEN
		| RCC_AHBENR_ETHMACRXEN
		;
	RCC->APB1ENR |= 0
		| RCC_APB1ENR_SPI3EN
		;
	RCC->APB2ENR |= 0
		| RCC_APB2ENR_AFIOEN
		| RCC_APB2ENR_IOPAEN
		| RCC_APB2ENR_IOPBEN
		| RCC_APB2ENR_IOPCEN
		| RCC_APB2ENR_IOPDEN
		;

	/* Pin mapping */
	AFIO->MAPR |= 0
		| AFIO_MAPR_TIM3_REMAP_FULLREMAP
		| AFIO_MAPR_SWJ_CFG_JTAGDISABLE
		| AFIO_MAPR_MII_RMII_SEL
		;

	setup_bank(GPIOA, gpio_cfg[0]);
	setup_bank(GPIOB, gpio_cfg[1]);
	setup_bank(GPIOC, gpio_cfg[2]);
	setup_bank(GPIOD, gpio_cfg[3]);

	/* Configure clocking */
	RCC->CR |= RCC_CR_HSEBYP | RCC_CR_HSEON;
	while (!(RCC->CR & RCC_CR_HSERDY)) {}
	RCC->CFGR = 0
		| RCC_CFGR_PLLSRC_PREDIV1 \
		| RCC_CFGR_HPRE_DIV1 \
		| RCC_CFGR_PPRE1_DIV2 \
		| RCC_CFGR_PPRE2_DIV1 \
		| RCC_CFGR_ADCPRE_DIV8;
	RCC->CFGR2 = RCC_CFGR2_PREDIV1SRC_HSE;
	FLASH->ACR = FLASH_ACR_LATENCY_2 | FLASH_ACR_PRFTBE;
}
