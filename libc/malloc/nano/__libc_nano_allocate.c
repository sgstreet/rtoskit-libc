/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_nano_allocate.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */
#include <stddef.h>

#include "nano.h"

/* First fit; a large enough remainder stays in the list in the chunk's place */
static void *find(size_t stride)
{
	for (struct __libc_nano_chunk **link = &__libc_nano_heap.free_list; *link != nullptr; link = &(*link)->next) {
		struct __libc_nano_chunk *chunk = *link;
		size_t size = *__libc_nano_header(chunk);

		if (size < stride)
			continue;

		if (size - stride >= __LIBC_NANO_MIN_STRIDE) {
			struct __libc_nano_chunk *rest = (struct __libc_nano_chunk *)((char *)chunk + stride);

			*__libc_nano_header(rest) = size - stride;
			rest->next = chunk->next;
			*link = rest;
			*__libc_nano_header(chunk) = stride;
		} else {
			*link = chunk->next;
		}

		return chunk;
	}

	return nullptr;
}

/* Add a chunk of at least stride bytes from __rtos_sbrk, merged with a free top chunk when in place (D103) */
static bool grow(size_t stride)
{
	struct __libc_malloc_heap *top = &__libc_nano_heap.top;
	char *bridge;
	char *chunk;

	if (!__libc_malloc_grow(top, stride, __LIBC_NANO_MIN_STRIDE, &chunk, &bridge))
		return false;

	/* The old end becomes a used bridge over memory that is not the heap's */
	*__libc_nano_header(bridge) = chunk - bridge;
	*__libc_nano_header(chunk) = top->end - chunk;
	__libc_nano_release(chunk);

	return true;
}

/* Out of line, so that the first-fit path saves no registers for it */
[[gnu::noinline]] static void *grow_and_allocate(size_t stride)
{
	return grow(stride) ? __libc_nano_allocate(stride) : nullptr;
}

void *__libc_nano_allocate(size_t stride)
{
	void *p = find(stride);

	return p != nullptr ? p : grow_and_allocate(stride);
}
