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

#include "linux.h"

/*
 * The break moves inside one reservation of address space, mapped at the
 * first call; untouched pages cost nothing. libc calls this with
 * __rtos_malloc_lock held, and nothing else in the port moves the break.
 */
static struct {
	uintptr_t brk;
	uintptr_t end;
} heap;

void *__rtos_sbrk(ptrdiff_t increment)
{
	uintptr_t previous;

	if (heap.end == 0) {
		long region = __linux_syscall6(__LINUX_SYS_MMAP, 0, (long)__LINUX_HEAP_SIZE, __LINUX_PROT_READ | __LINUX_PROT_WRITE, __LINUX_MAP_PRIVATE | __LINUX_MAP_ANONYMOUS | __LINUX_MAP_NORESERVE, -1, 0);

		if (__linux_is_error(region))
			return (void *)-1;
		heap.brk = (uintptr_t)region;
		heap.end = heap.brk + __LINUX_HEAP_SIZE;
	}

	/* libc only grows the heap (rtos-port.h): a negative increment fails like one past the end */
	if ((uintptr_t)increment > heap.end - heap.brk)
		return (void *)-1;

	previous = heap.brk;
	heap.brk += increment;
	return (void *)previous;
}
