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

	sl_map = heap->sl_bitmap[fl] & (UINT32_MAX << sl);
	if (sl_map == 0) {
		fl_map = fl + 1 < __LIBC_TLSF_FL_COUNT ? heap->fl_bitmap & (SIZE_MAX << (fl + 1)) : 0;
		if (fl_map == 0)
			return nullptr;
		fl = stdc_trailing_zeros(fl_map);
		sl_map = heap->sl_bitmap[fl];
	}

	return heap->lists[fl][stdc_trailing_zeros(sl_map)];
}

/* The free block that ends at the sentinel, or nullptr */
static void *free_top(struct __libc_tlsf *heap)
{
	char *end = heap->top.end;

	return end != nullptr && __libc_tlsf_prev_is_free(end) ? *__libc_tlsf_back_pointer(end) : nullptr;
}

/* Add a block of at least search bytes from __rtos_sbrk, merged with a free top block when in place (D103) */
static bool grow(struct __libc_tlsf *heap, size_t search)
{
	struct __libc_malloc_heap *top = &heap->top;
	char *end = top->end;
	size_t prev_flag = end != nullptr ? *__libc_tlsf_header(end) & __LIBC_TLSF_PREV_FREE : 0;
	char *bridge;
	char *chunk;

	if (!__libc_malloc_grow(top, search, __LIBC_TLSF_MIN_STRIDE, &chunk, &bridge))
		return false;

	/* The old sentinel becomes a used bridge over memory that is not the heap's */
	if (bridge != chunk) {
		*__libc_tlsf_header(end) = (size_t)(chunk - end) | prev_flag;
		prev_flag = 0;
	}

	/* A used block that release merges with a free top block and files */
	*__libc_tlsf_header(top->end) = 0;
	*__libc_tlsf_header(chunk) = (size_t)(top->end - chunk) | prev_flag;
	__libc_tlsf_release(heap, chunk);

	return true;
}

/* Take a block of at least stride bytes from the free lists, growing the heap when none fits, and mark it used */
void *__libc_tlsf_allocate(struct __libc_tlsf *heap, size_t stride)
{
	size_t search = round_to_class(stride);
	unsigned int fl;
	unsigned int sl;
	void *p;

	if (search == 0)
		return nullptr;

	__libc_tlsf_mapping(search, &fl, &sl);
	while ((p = find(heap, fl, sl)) == nullptr) {
		if (!grow(heap, search)) {
			/* No class-sized block and no more memory: a free top block may still hold stride exactly */
			p = free_top(heap);
			if (p == nullptr || __libc_tlsf_stride(p) < stride)
				return nullptr;
			break;
		}
	}

	__libc_tlsf_remove(heap, p);
	*__libc_tlsf_header(p) &= ~(size_t)__LIBC_TLSF_FREE;
	*__libc_tlsf_header(__libc_tlsf_next(p)) &= ~(size_t)__LIBC_TLSF_PREV_FREE;

	__libc_tlsf_trim(heap, p, stride);
	return p;
}
