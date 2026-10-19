/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_malloc_check.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <rtos/rtos-port.h>

#include "internal/malloc.h"
#include "nano.h"

/* Walk one region chunk by chunk, matching the free chunks inside it against the list */
static bool check_region(struct __libc_nano_region *region, size_t *matched)
{
	struct __libc_nano_chunk *free_chunk = __libc_nano_heap.free_list;
	char *p = region->first;
	char *end = region->end;
	bool prev_free = false;

	while (free_chunk != nullptr && __libc_nano_below(free_chunk, p))
		free_chunk = free_chunk->next;

	while (p != end) {
		size_t stride = *__libc_nano_header(p);
		bool is_free = (char *)free_chunk == p;

		if (stride < __LIBC_NANO_MIN_STRIDE || stride % __LIBC_NANO_ALIGN != 0 || stride > (size_t)(end - p))
			return false;

		/* Free neighbours are always merged */
		if (is_free && prev_free)
			return false;
		if (is_free) {
			++*matched;
			free_chunk = free_chunk->next;
		}

		prev_free = is_free;
		p += stride;
	}

	/* A list entry inside the region that is not a chunk boundary */
	return free_chunk == nullptr || !__libc_nano_below(free_chunk, end);
}

bool __libc_malloc_check(void)
{
	size_t listed = 0;
	size_t matched = 0;
	bool status = true;

	__rtos_lock_acquire(&__rtos_malloc_lock);
	__libc_nano_init();

	/* The list is in strictly ascending address order */
	for (struct __libc_nano_chunk *chunk = __libc_nano_heap.free_list; chunk != nullptr; chunk = chunk->next) {
		if (chunk->next != nullptr && !__libc_nano_below(chunk, chunk->next))
			status = false;
		++listed;
	}

	for (struct __libc_nano_region *region = __libc_nano_heap.regions; status && region != nullptr; region = region->next)
		status = check_region(region, &matched);
	__rtos_lock_release(&__rtos_malloc_lock);

	return status && matched == listed;
}
