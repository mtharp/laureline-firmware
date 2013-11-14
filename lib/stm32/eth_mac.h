/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _ETH_MAC_H
#define _ETH_MAC_H

#define MAC_BUF_SIZE 1522
#define SMI_DESCRIBE_SIZE 17

typedef struct mac_desc {
	volatile uint32_t des0;
	volatile uint32_t des1;
	uint8_t *des_buf;
	struct mac_desc *des_next;

	uint32_t size;
	uint32_t offset;
} mac_desc_t;

extern OS_FlagID mac_rx_flag, mac_tx_flag;

void smi_write(uint32_t reg, uint32_t value);
uint32_t smi_read(uint32_t reg);
uint8_t smi_poll_link_status(void);
void smi_describe_link(char *buf);

void mac_start(void);
void mac_set_hwaddr(const uint8_t *hwaddr);
mac_desc_t *mac_get_tx_descriptor(uint32_t timeout);
uint16_t mac_write_tx_descriptor(mac_desc_t *tdes, const uint8_t *buf, uint16_t size);
void mac_release_tx_descriptor(mac_desc_t *tdes);
mac_desc_t *mac_get_rx_descriptor(void);
uint16_t mac_read_rx_descriptor(mac_desc_t *rdes, uint8_t *buf, uint16_t size);
void mac_release_rx_descriptor(mac_desc_t *rdes);


#define STM32_RDES0_OWN             0x80000000
#define STM32_RDES0_AFM             0x40000000
#define STM32_RDES0_FL_MASK         0x3FFF0000
#define STM32_RDES0_ES              0x00008000
#define STM32_RDES0_DESERR          0x00004000
#define STM32_RDES0_SAF             0x00002000
#define STM32_RDES0_LE              0x00001000
#define STM32_RDES0_OE              0x00000800
#define STM32_RDES0_VLAN            0x00000400
#define STM32_RDES0_FS              0x00000200
#define STM32_RDES0_LS              0x00000100
#define STM32_RDES0_IPHCE           0x00000080
#define STM32_RDES0_LCO             0x00000040
#define STM32_RDES0_FT              0x00000020
#define STM32_RDES0_RWT             0x00000010
#define STM32_RDES0_RE              0x00000008
#define STM32_RDES0_DE              0x00000004
#define STM32_RDES0_CE              0x00000002
#define STM32_RDES0_PCE             0x00000001

#define STM32_RDES1_DIC             0x80000000
#define STM32_RDES1_RBS2_MASK       0x1FFF0000
#define STM32_RDES1_RER             0x00008000
#define STM32_RDES1_RCH             0x00004000
#define STM32_RDES1_RBS1_MASK       0x00001FFF

#define STM32_TDES0_OWN             0x80000000
#define STM32_TDES0_IC              0x40000000
#define STM32_TDES0_LS              0x20000000
#define STM32_TDES0_FS              0x10000000
#define STM32_TDES0_DC              0x08000000
#define STM32_TDES0_DP              0x04000000
#define STM32_TDES0_TTSE            0x02000000
#define STM32_TDES0_LOCKED          0x01000000 /* NOTE: Pseudo flag.        */
#define STM32_TDES0_CIC_MASK        0x00C00000
#define STM32_TDES0_CIC(n)          ((n) << 22)
#define STM32_TDES0_TER             0x00200000
#define STM32_TDES0_TCH             0x00100000
#define STM32_TDES0_TTSS            0x00020000
#define STM32_TDES0_IHE             0x00010000
#define STM32_TDES0_ES              0x00008000
#define STM32_TDES0_JT              0x00004000
#define STM32_TDES0_FF              0x00002000
#define STM32_TDES0_IPE             0x00001000
#define STM32_TDES0_LCA             0x00000800
#define STM32_TDES0_NC              0x00000400
#define STM32_TDES0_LCO             0x00000200
#define STM32_TDES0_EC              0x00000100
#define STM32_TDES0_VF              0x00000080
#define STM32_TDES0_CC_MASK         0x00000078
#define STM32_TDES0_ED              0x00000004
#define STM32_TDES0_UF              0x00000002
#define STM32_TDES0_DB              0x00000001

#define STM32_TDES1_TBS2_MASK       0x1FFF0000
#define STM32_TDES1_TBS1_MASK       0x00001FFF

#define STM32_IP_CHECKSUM_OFFLOAD   3

#endif
