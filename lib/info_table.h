/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _INFO_TABLE_H
#define _INFO_TABLE_H

typedef struct {
    uint32_t type;
    void *ptr;
} info_entry_t;

extern const info_entry_t boot_table[], app_table[];

const void *info_get(const info_entry_t *table, uint32_t type);

#define INFO_APPVER             0x4d4e5641 /* AVNM */
#define INFO_BOOTVER            0x4d4e5642 /* BVNM */
#define INFO_HWVER              0x4d4e5648 /* HVNM */
#define INFO_HSE_FREQ           0x46455348 /* HSEF */
#define INFO_END                0

#endif
