/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_tlsf_allocate.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <stdint.h>

#include "internal/malloc.h"
#include "tlsf.h"

/*
 * Round a stride up to the first size of the next list class, so that the
 * head of any non-empty list from that class on is big enough (good fit).
 * Returns 0 on overflow.
 */
static size_t round_to_class(size_t stride)
{
	size_t step;

	if (stride < __LIBC_TLSF_SMALL)
		return stride;

	step = (size_t)1 << (__libc_tlsf_log2(stride) - __LIBC_TLSF_SL_LOG2);
	if (stride > SIZE_MAX - (step - 1))
		return 0;

	return (stride + step - 1) & ~(step - 1);
}

/* The first non-empty list at or above (fl, sl), or nullptr: two bitmap lookups */
static struct __libc_free_block *find(struct __libc_tlsf *heap, unsigned int fl, unsigned int sl)
{
	uint32_t sl_map;
	size_t fl_map;

	if (fl >= heap->fl_count)
		return nullptr;

	sl_map = heap->sl_bitmap[fl] & (UINT32_MAX << sl);
	if (sl_map == 0) {
		fl_map = fl + 1 < __LIBC_TLSF_FL_MAX ? heap->fl_bitmap & (SIZE_MAX << (fl + 1)) : 0;
		if (fl_map == 0)
			return nullptr;
		fl = stdc_trailing_zeros(fl_map);
		sl_map = heap->sl_bitmap[fl];
	}

	return heap->lists[fl][stdc_trailing_zeros(sl_map)];
}

/* Take a block of at least stride bytes from the free lists and mark it used */
void *__libc_tlsf_allocate(struct __libc_tlsf *heap, size_t stride)
{
	size_t search = round_to_class(stride);
	unsigned int fl;
	unsigned int sl;
	void *p;

	if (search == 0)
		return nullptr;

	__libc_tlsf_mapping(search, &fl, &sl);
	p = find(heap, fl, sl);
	if (p == nullptr)
		return nullptr;

	__libc_tlsf_remove(heap, p);
	*__libc_tlsf_header(p) &= ~(size_t)__LIBC_TLSF_FREE;
	*__libc_tlsf_header(__libc_tlsf_next(p)) &= ~(size_t)__LIBC_TLSF_PREV_FREE;

	__libc_tlsf_trim(heap, p, stride);
	return p;
}
