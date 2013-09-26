/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "diskio.h"


DSTATUS
disk_initialize (BYTE pdrv) {
	return STA_NOINIT;
}


DSTATUS
disk_status (BYTE pdrv) {
	return STA_NOINIT;
}


DRESULT
disk_read (BYTE pdrv, BYTE *buff, DWORD sector, BYTE count) {
	return RES_PARERR;
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
