/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_heap_region.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <rtos/rtos-port.h>

#include "rtos-none.h"

/* One region between the linker script symbols, if the board defines them */
[[gnu::weak]] bool __rtos_heap_region(unsigned int index, void **base, size_t *size)
{
	uintptr_t start = (uintptr_t)__rtos_none_heap_start;
	uintptr_t end = (uintptr_t)__rtos_none_heap_end;

	if (index != 0 || start == 0 || end <= start)
		return false;

	*base = __rtos_none_heap_start;
	*size = end - start;
	return true;
}
