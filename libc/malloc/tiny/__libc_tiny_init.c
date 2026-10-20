/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_tiny_init.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "tiny.h"

/*
 * The port's first heap region becomes one free chunk followed by the
 * sentinel, once. Later regions are not used: that is what keeps the tiny
 * allocator tiny (PLAN D54).
 */
void __libc_tiny_init(void)
{
	void *base;
	size_t size;

	if (__libc_tiny_heap.initialised)
		return;
	__libc_tiny_heap.initialised = true;

	if (!__rtos_heap_region(0, &base, &size) || size < 4 * __LIBC_TINY_ALIGN)
		return;

	uintptr_t first = ((uintptr_t)base + __LIBC_TINY_WORD + __LIBC_TINY_ALIGN - 1) & ~(uintptr_t)(__LIBC_TINY_ALIGN - 1);
	uintptr_t sentinel = (((uintptr_t)base + size) & ~(uintptr_t)(__LIBC_TINY_ALIGN - 1)) - __LIBC_TINY_ALIGN;

	*__libc_tiny_header((void *)first) = sentinel - first;
	*__libc_tiny_header((void *)sentinel) = __LIBC_TINY_USED;
	__libc_tiny_heap.first = (char *)first;
}
