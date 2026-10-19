/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_tlsf_release.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>

#include "internal/malloc.h"
#include "tlsf.h"

/* Free the used block p, merging it with free neighbours (no two free blocks are ever adjacent) */
void __libc_tlsf_release(struct __libc_tlsf *heap, void *p)
{
	size_t stride = __libc_tlsf_stride(p);
	size_t prev_flag = *__libc_tlsf_header(p) & __LIBC_TLSF_PREV_FREE;
	void *next = (char *)p + stride;

	if (__libc_tlsf_is_free(next)) {
		__libc_tlsf_remove(heap, next);
		stride += __libc_tlsf_stride(next);
	}

	if (prev_flag != 0) {
		void *prev = *__libc_tlsf_back_pointer(p);

		__libc_tlsf_remove(heap, prev);
		stride += __libc_tlsf_stride(prev);
		prev_flag = *__libc_tlsf_header(prev) & __LIBC_TLSF_PREV_FREE;
		p = prev;
	}

	*__libc_tlsf_header(p) = stride | __LIBC_TLSF_FREE | prev_flag;
	next = (char *)p + stride;
	*__libc_tlsf_back_pointer(next) = p;
	*__libc_tlsf_header(next) |= __LIBC_TLSF_PREV_FREE;
	__libc_tlsf_insert(heap, p);
}
