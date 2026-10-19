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
void *__libc_nano_allocate(size_t stride)
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
