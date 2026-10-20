/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * aligned_alloc.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <stdbit.h>
#include <stdlib.h>

#include "tiny.h"

/*
 * C23 7.24.3.1: any power of two is a valid alignment. Over-allocate by the
 * alignment plus a minimum chunk, leave the gap before the aligned payload
 * as a free chunk (it is either empty or big enough to be one) and trim the
 * tail.
 */
void *aligned_alloc(size_t alignment, size_t size)
{
	size_t stride = __libc_tiny_request(size);
	uintptr_t address;
	void *p = nullptr;

	if (!stdc_has_single_bit(alignment)) {
		errno = EINVAL;
		return nullptr;
	}
	if (alignment <= __LIBC_TINY_ALIGN)
		return malloc(size);

	if (stride == 0 || stride > SIZE_MAX - alignment - __LIBC_TINY_MIN_STRIDE) {
		errno = ENOMEM;
		return nullptr;
	}

	__rtos_lock_acquire(&__rtos_malloc_lock);
	__libc_tiny_init();
	p = __libc_tiny_allocate(stride + alignment + __LIBC_TINY_MIN_STRIDE);
	if (p != nullptr) {
		address = ((uintptr_t)p + alignment - 1) & ~(uintptr_t)(alignment - 1);

		/* A gap smaller than a chunk cannot be freed; the next aligned address leaves at least alignment */
		if (address != (uintptr_t)p && address - (uintptr_t)p < __LIBC_TINY_MIN_STRIDE)
			address += alignment;

		if (address != (uintptr_t)p) {
			size_t gap = address - (uintptr_t)p;

			*__libc_tiny_header((void *)address) = (__libc_tiny_stride(p) - gap) | __LIBC_TINY_USED;
			*__libc_tiny_header(p) = gap;
			p = (void *)address;
		}

		__libc_tiny_trim(p, stride);
	}
	__rtos_lock_release(&__rtos_malloc_lock);

	if (p == nullptr)
		errno = ENOMEM;

	return p;
}
