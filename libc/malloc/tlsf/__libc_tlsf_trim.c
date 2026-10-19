/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_tlsf_trim.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>

#include "internal/malloc.h"
#include "tlsf.h"

/* Shrink the used block p to stride, releasing the tail when it can form a block */
void __libc_tlsf_trim(struct __libc_tlsf *heap, void *p, size_t stride)
{
	size_t current = __libc_tlsf_stride(p);
	void *tail;

	if (current - stride < __LIBC_TLSF_MIN_STRIDE)
		return;

	*__libc_tlsf_header(p) = stride | (*__libc_tlsf_header(p) & __LIBC_TLSF_PREV_FREE);
	tail = (char *)p + stride;
	*__libc_tlsf_header(tail) = current - stride;
	__libc_tlsf_release(heap, tail);
}
