/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "info_table.h"
#include "version.h"


const info_entry_t boot_table[] = {
    {INFO_BOOTVER, VERSION},
    {INFO_HWVER, (void*)HW_VERSION},
    {INFO_HSE_FREQ, (void*)HSE_FREQ},
    {INFO_END, NULL},
};
