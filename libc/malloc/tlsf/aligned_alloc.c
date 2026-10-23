/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * aligned_alloc.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdbit.h>
#include <stdint.h>
#include <stdlib.h>
#include <rtos/rtos-port.h>

#include "internal/malloc.h"
#include "tlsf.h"

/*
 * C23 7.24.3.1: any power of two is a valid alignment. Over-allocate by the
 * alignment plus a minimum block, then free the gap before the aligned
 * payload (it is either empty or big enough to be a block) and trim the tail.
 */
void *aligned_alloc(size_t alignment, size_t size)
{
	size_t stride = __libc_tlsf_request(size);
	size_t total;
	struct __libc_tlsf *heap = &__libc_tlsf_heap;
	uintptr_t address;
	void *p = nullptr;

	if (!stdc_has_single_bit(alignment)) {
		errno = EINVAL;
		return nullptr;
	}
	if (alignment <= __LIBC_TLSF_ALIGN)
		return malloc(size);

	if (stride == 0 || alignment > __LIBC_MALLOC_MAX) {
		errno = ENOMEM;
		return nullptr;
	}
	total = stride + alignment + __LIBC_TLSF_MIN_STRIDE;

	__rtos_lock_acquire(&__rtos_malloc_lock);
	p = __libc_tlsf_allocate(heap, total);

	if (p != nullptr) {
		address = ((uintptr_t)p + alignment - 1) & ~(uintptr_t)(alignment - 1);

		/* A gap smaller than a block cannot be freed; the next aligned address leaves at least alignment */
		if (address != (uintptr_t)p && address - (uintptr_t)p < __LIBC_TLSF_MIN_STRIDE)
			address += alignment;

		if (address != (uintptr_t)p) {
			size_t gap = address - (uintptr_t)p;
			size_t current = __libc_tlsf_stride(p);

			*__libc_tlsf_header(p) = gap | (*__libc_tlsf_header(p) & __LIBC_TLSF_PREV_FREE);
			*__libc_tlsf_header((void *)address) = current - gap;
			__libc_tlsf_release(heap, p);
			p = (void *)address;
		}

		__libc_tlsf_trim(heap, p, stride);
	}
	__rtos_lock_release(&__rtos_malloc_lock);

	if (p == nullptr)
		errno = ENOMEM;

	return p;
}
