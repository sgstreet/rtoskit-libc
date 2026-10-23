/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_sbrk.c
 *
 * Created on: Oct 23, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <rtos/rtos-port.h>

#include "rtos-none.h"

/*
 * The break moves between the linker script symbols, if the board defines
 * them. Single-threaded: an interrupt handler that allocates is not
 * supported. Weak, so a board can supply its own sbrk.
 */
static char *brk;

[[gnu::weak]] void *__rtos_sbrk(ptrdiff_t increment)
{
	uintptr_t start = (uintptr_t)__rtos_none_heap_start;
	uintptr_t end = (uintptr_t)__rtos_none_heap_end;
	uintptr_t current;

	if (start == 0 || end <= start)
		return (void *)-1;
	if (brk == nullptr)
		brk = __rtos_none_heap_start;

	/* libc only grows the heap (rtos-port.h): a negative increment fails like one past the end */
	current = (uintptr_t)brk;
	if ((uintptr_t)increment > end - current)
		return (void *)-1;

	brk += increment;
	return (void *)current;
}
