/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include <stddef.h>
#include <stdint.h>
#include "info_table.h"

const void *
info_get(const info_entry_t *table, uint32_t type) {
	for (; table->type != 0; table++) {
		if (table->type == type) {
			return table->ptr;
		}
	}
	return NULL;
}
