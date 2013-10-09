/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "diskio.h"
#include "stm32/mmc.h"


DSTATUS
disk_initialize (BYTE pdrv) {
	if (mmc_state == MMC_UNLOADED) {
		return STA_NODISK;
	} else {
		return 0;
	}
}


DSTATUS
disk_status (BYTE pdrv) {
	if (mmc_state == MMC_UNLOADED) {
		return STA_NODISK;
	} else {
		return 0;
	}
}


DRESULT
disk_read (BYTE pdrv, BYTE *buff, DWORD sector, BYTE count) {
	if (mmc_state != MMC_READY) {
		return RES_NOTRDY;
	}
	if (mmc_start_read(sector)) {
		return RES_ERROR;
	}
	while (count > 0) {
		if (mmc_read_sector(buff)) {
			return RES_ERROR;
		}
		buff += 512;
		count--;
	}
	if (mmc_stop_read()) {
		return RES_ERROR;
	}
	return RES_OK;
}


DRESULT
disk_write (BYTE pdrv, const BYTE *buff, DWORD sector, BYTE count) {
	return RES_PARERR;
}


#if _USE_IOCTL
DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void *buff) {
	return RES_PARERR;
}
#endif
