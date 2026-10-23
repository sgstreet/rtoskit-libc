/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * realloc.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <rtos/rtos-port.h>

#include "internal/malloc.h"
#include "tlsf.h"

/*
 * C23 7.24.3.7: shrink in place, grow into a free successor, or move. A size
 * of zero is undefined behaviour; it shrinks to a minimum block like
 * malloc(0) (J.3.12). On failure the old object is left untouched.
 */
void *realloc(void *ptr, size_t size)
{
	size_t stride;
	size_t current;
	struct __libc_tlsf *heap;
	void *next;
	void *moved = nullptr;

	if (ptr == nullptr)
		return malloc(size);

	stride = __libc_tlsf_request(size);
	if (stride == 0) {
		errno = ENOMEM;
		return nullptr;
	}

	__rtos_lock_acquire(&__rtos_malloc_lock);
	heap = &__libc_tlsf_heap;
	current = __libc_tlsf_stride(ptr);

	if (stride > current) {
		next = (char *)ptr + current;
		if (__libc_tlsf_is_free(next) && current + __libc_tlsf_stride(next) >= stride) {
			/* Absorb the free successor; the block after it now follows a used block */
			__libc_tlsf_remove(heap, next);
			current += __libc_tlsf_stride(next);
			*__libc_tlsf_header(ptr) = current | (*__libc_tlsf_header(ptr) & __LIBC_TLSF_PREV_FREE);
			*__libc_tlsf_header(__libc_tlsf_next(ptr)) &= ~(size_t)__LIBC_TLSF_PREV_FREE;
		} else {
			moved = __libc_tlsf_allocate(heap, stride);
			if (moved != nullptr) {
				memcpy(moved, ptr, current - __LIBC_TLSF_WORD);
				__libc_tlsf_release(heap, ptr);
			}
			__rtos_lock_release(&__rtos_malloc_lock);

			if (moved == nullptr)
				errno = ENOMEM;
			return moved;
		}
	}

	__libc_tlsf_trim(heap, ptr, stride);
	__rtos_lock_release(&__rtos_malloc_lock);

	return ptr;
}
