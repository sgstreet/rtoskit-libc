/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_nano_release.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>

#include "nano.h"

/* Insert in address order, merging with the neighbours it touches */
void __libc_nano_release(void *p)
{
	struct __libc_nano_chunk *chunk = p;
	struct __libc_nano_chunk *prev = nullptr;
	struct __libc_nano_chunk **link = &__libc_nano_heap.free_list;
	struct __libc_nano_chunk *next;
	size_t size = *__libc_nano_header(p);

	while (*link != nullptr && __libc_nano_below(*link, chunk)) {
		prev = *link;
		link = &prev->next;
	}
	next = *link;

	if (next != nullptr && (char *)chunk + size == (char *)next) {
		size += *__libc_nano_header(next);
		next = next->next;
	}

	if (prev != nullptr && (char *)prev + *__libc_nano_header(prev) == (char *)chunk) {
		*__libc_nano_header(prev) += size;
		prev->next = next;
		return;
	}

	*__libc_nano_header(chunk) = size;
	chunk->next = next;
	*link = chunk;
}
