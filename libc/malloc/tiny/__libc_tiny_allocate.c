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

/* First fit from the first chunk, merging each free chunk with the free chunks after it before trying it */
void *__libc_tiny_allocate(size_t stride)
{
	char *p = __libc_tiny_heap.first;

	if (p == nullptr)
		return nullptr;

	for (; __libc_tiny_stride(p) != 0; p += __libc_tiny_stride(p)) {
		if (*__libc_tiny_header(p) & __LIBC_TINY_USED)
			continue;

		__libc_tiny_merge(p);
		if (*__libc_tiny_header(p) >= stride) {
			*__libc_tiny_header(p) |= __LIBC_TINY_USED;
			__libc_tiny_trim(p, stride);
			return p;
		}
	}

	return nullptr;
}
