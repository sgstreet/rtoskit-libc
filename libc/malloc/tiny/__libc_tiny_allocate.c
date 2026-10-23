/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_tiny_allocate.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "tiny.h"

/* A free chunk of at least stride bytes from __rtos_sbrk, or nullptr; a later walk merges it with a free top chunk (D103) */
static char *grow(size_t stride)
{
	struct __libc_malloc_heap *top = &__libc_tiny_heap.top;
	char *bridge;
	char *chunk;

	if (!__libc_malloc_grow(top, stride, __LIBC_TINY_MIN_STRIDE, &chunk, &bridge))
		return nullptr;

	/* The old sentinel becomes a used bridge over memory that is not the heap's */
	*__libc_tiny_header(bridge) = (size_t)(chunk - bridge) | __LIBC_TINY_USED;
	*__libc_tiny_header(top->end) = __LIBC_TINY_USED;
	*__libc_tiny_header(chunk) = top->end - chunk;

	return chunk;
}

/*
 * First fit from the first chunk, merging each free chunk with the free
 * chunks after it before trying it; when none fits, the new chunk from the
 * break holds the stride, so it is taken without walking again.
 */
void *__libc_tiny_allocate(size_t stride)
{
	char *p = __libc_tiny_heap.top.first;

	/* The empty heap is tested once, not at every step of the walk */
	if (p != nullptr) {
		for (; __libc_tiny_stride(p) != 0; p += __libc_tiny_stride(p)) {
			if (*__libc_tiny_header(p) & __LIBC_TINY_USED)
				continue;
			__libc_tiny_merge(p);
			if (*__libc_tiny_header(p) >= stride)
				break;
		}
	}

	if (p == nullptr || __libc_tiny_stride(p) == 0) {
		p = grow(stride);
		if (p == nullptr)
			return nullptr;
	}

	*__libc_tiny_header(p) |= __LIBC_TINY_USED;
	__libc_tiny_trim(p, stride);
	return p;
}
