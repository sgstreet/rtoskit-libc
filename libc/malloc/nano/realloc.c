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

#include "nano.h"

/* Grow p in place to at least stride if the chunk after it is free and big enough */
static bool grow(void *p, size_t stride)
{
	size_t size = *__libc_nano_header(p);
	struct __libc_nano_chunk *next = (struct __libc_nano_chunk *)((char *)p + size);
	struct __libc_nano_chunk **link = &__libc_nano_heap.free_list;

	while (*link != nullptr && __libc_nano_below(*link, next))
		link = &(*link)->next;

	if (*link != next || size + *__libc_nano_header(next) < stride)
		return false;

	*link = next->next;
	*__libc_nano_header(p) = size + *__libc_nano_header(next);
	return true;
}

/*
 * C23 7.24.3.7: shrink in place, grow into a free successor, or move. A size
 * of zero is undefined behaviour; it shrinks to a minimum chunk like
 * malloc(0) (J.3.12). On failure the old object is left untouched.
 */
void *realloc(void *ptr, size_t size)
{
	size_t stride;
	void *moved;

	if (ptr == nullptr)
		return malloc(size);

	stride = __libc_nano_request(size);
	if (stride == 0) {
		errno = ENOMEM;
		return nullptr;
	}

	__rtos_lock_acquire(&__rtos_malloc_lock);
	if (stride <= *__libc_nano_header(ptr) || grow(ptr, stride)) {
		__libc_nano_trim(ptr, stride);
		__rtos_lock_release(&__rtos_malloc_lock);
		return ptr;
	}

	moved = __libc_nano_allocate(stride);
	if (moved != nullptr) {
		memcpy(moved, ptr, *__libc_nano_header(ptr) - __LIBC_NANO_WORD);
		__libc_nano_release(ptr);
	}
	__rtos_lock_release(&__rtos_malloc_lock);

	if (moved == nullptr)
		errno = ENOMEM;

	return moved;
}
