/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _MMC_H
#define _MMC_H

#include "common.h"

typedef enum {
	MMC_UNLOADED = 0,
	MMC_READY,
	MMC_READING,
	MMC_WRITING
} mmc_state_t;

extern mmc_state_t mmc_state;

void mmc_start(void);
void mmc_sync(void);
int16_t mmc_connect(void);
int16_t mmc_disconnect(void);
int16_t mmc_start_read(uint32_t lba);
int16_t mmc_read_sector(uint8_t *out);
int16_t mmc_stop_read(void);

#define MMC_RESET_DEADLINE			MS2ST(100)
#define MMC_INIT_DEADLINE			MS2ST(1000)
#define MMC_DATA_DEADLINE			MS2ST(100)
#define MMC_IDLE_DEADLINE			MS2ST(1000)

#define MMC_CMDGOIDLE				0
#define MMC_CMDINIT					1
#define MMC_CMDINTERFACE_CONDITION	8
#define MMC_CMDREADCSD				9
#define MMC_CMDSTOP					12
#define MMC_CMDSETBLOCKLEN			16
#define MMC_CMDREAD					17
#define MMC_CMDREADMULTIPLE			18
#define MMC_CMDWRITE				24
#define MMC_CMDWRITEMULTIPLE		25
#define MMC_CMDAPP					55
#define MMC_CMDREADOCR				58
#define MMC_ACMDOPCONDITION			41

#endif
