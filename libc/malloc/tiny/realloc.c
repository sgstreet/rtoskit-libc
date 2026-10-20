/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * realloc.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <stdlib.h>
#include <string.h>

#include "tiny.h"

/*
 * C23 7.24.3.7: shrink in place, grow over the free chunks that follow, or
 * move. A size of zero is undefined behaviour; it shrinks to a minimum chunk
 * like malloc(0) (J.3.12). On failure the old object is left untouched.
 */
void *realloc(void *ptr, size_t size)
{
	size_t stride;
	void *moved;

	if (ptr == nullptr)
		return malloc(size);

	stride = __libc_tiny_request(size);
	if (stride == 0) {
		errno = ENOMEM;
		return nullptr;
	}

	__rtos_lock_acquire(&__rtos_malloc_lock);
	if (stride > __libc_tiny_stride(ptr))
		__libc_tiny_merge(ptr);
	if (stride <= __libc_tiny_stride(ptr)) {
		__libc_tiny_trim(ptr, stride);
		__rtos_lock_release(&__rtos_malloc_lock);
		return ptr;
	}

	moved = __libc_tiny_allocate(stride);
	if (moved != nullptr) {
		memcpy(moved, ptr, __libc_tiny_stride(ptr) - __LIBC_TINY_WORD);
		*__libc_tiny_header(ptr) &= ~__LIBC_TINY_USED;
	}
	__rtos_lock_release(&__rtos_malloc_lock);

	if (moved == nullptr)
		errno = ENOMEM;

	return moved;
}
