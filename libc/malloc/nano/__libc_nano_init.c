/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_nano_init.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <rtos/rtos-port.h>

#include "nano.h"

/* Turn every region the port offers into one free chunk, once */
void __libc_nano_init(void)
{
	void *base;
	size_t size;

	if (__libc_nano_heap.initialised)
		return;
	__libc_nano_heap.initialised = true;

	for (unsigned int index = 0; __rtos_heap_region(index, &base, &size); ++index) {
		uintptr_t start = ((uintptr_t)base + alignof(struct __libc_nano_region) - 1) & ~(uintptr_t)(alignof(struct __libc_nano_region) - 1);
		uintptr_t first = (start + sizeof(struct __libc_nano_region) + __LIBC_NANO_WORD + __LIBC_NANO_ALIGN - 1) & ~(uintptr_t)(__LIBC_NANO_ALIGN - 1);
		uintptr_t end = ((uintptr_t)base + size) & ~(uintptr_t)(__LIBC_NANO_ALIGN - 1);
		struct __libc_nano_region *region = (struct __libc_nano_region *)start;

		if (size < 2 * sizeof(struct __libc_nano_region) + 2 * __LIBC_NANO_ALIGN || end <= first)
			continue;

		region->first = (void *)first;
		region->end = (void *)end;
		region->next = __libc_nano_heap.regions;
		__libc_nano_heap.regions = region;

		*__libc_nano_header(region->first) = end - first;
		__libc_nano_release(region->first);
	}
}
