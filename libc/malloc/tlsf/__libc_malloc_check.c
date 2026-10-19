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
#include "tlsf.h"

static bool in_list(struct __libc_tlsf *heap, void *p)
{
	unsigned int fl;
	unsigned int sl;

	__libc_tlsf_mapping(__libc_tlsf_stride(p), &fl, &sl);
	if (fl >= heap->fl_count)
		return false;

	for (struct __libc_free_block *block = heap->lists[fl][sl]; block != nullptr; block = block->next)
		if (block == p)
			return true;

	return false;
}

/* Every block of every region: strides, flags, merging and list membership */
static bool check_blocks(struct __libc_tlsf *heap, size_t *free_count)
{
	for (unsigned int r = 0; r < heap->region_count; ++r) {
		char *p = heap->regions[r].first;
		char *end = heap->regions[r].end;
		bool prev_free = false;

		while (p != end) {
			size_t stride = __libc_tlsf_stride(p);
			bool is_free = __libc_tlsf_is_free(p);

			if (stride < __LIBC_TLSF_MIN_STRIDE || stride % __LIBC_TLSF_ALIGN != 0 || stride > (size_t)(end - p))
				return false;
			if (__libc_tlsf_prev_is_free(p) != prev_free)
				return false;
			if (is_free && (prev_free || *__libc_tlsf_back_pointer(p + stride) != p || !in_list(heap, p)))
				return false;

			*free_count += is_free;
			prev_free = is_free;
			p += stride;
		}

		/* The sentinel: zero stride, used, and its flag mirrors the last block */
		if (__libc_tlsf_stride(end) != 0 || __libc_tlsf_is_free(end) || __libc_tlsf_prev_is_free(end) != prev_free)
			return false;
	}

	return true;
}

/* Every list: its blocks map to it, links are consistent, bitmaps match */
static bool check_lists(struct __libc_tlsf *heap, size_t *free_count)
{
	for (unsigned int fl = 0; fl < heap->fl_count; ++fl) {
		bool fl_used = false;

		for (unsigned int sl = 0; sl < __LIBC_TLSF_SL_COUNT; ++sl) {
			struct __libc_free_block *prev = nullptr;
			bool used = heap->lists[fl][sl] != nullptr;

			for (struct __libc_free_block *block = heap->lists[fl][sl]; block != nullptr; block = block->next) {
				unsigned int block_fl;
				unsigned int block_sl;

				__libc_tlsf_mapping(__libc_tlsf_stride(block), &block_fl, &block_sl);
				if (!__libc_tlsf_is_free(block) || block->prev != prev || block_fl != fl || block_sl != sl)
					return false;
				++*free_count;
				prev = block;
			}

			if (((heap->sl_bitmap[fl] >> sl) & 1) != used)
				return false;
			fl_used |= used;
		}

		if (((heap->fl_bitmap >> fl) & 1) != fl_used)
			return false;
	}

	return true;
}

bool __libc_malloc_check(void)
{
	struct __libc_tlsf *heap;
	size_t in_blocks = 0;
	size_t in_lists = 0;
	bool status = true;

	__rtos_lock_acquire(&__rtos_malloc_lock);
	heap = __libc_tlsf_get();
	if (heap != nullptr)
		status = check_blocks(heap, &in_blocks) && check_lists(heap, &in_lists) && in_blocks == in_lists;
	__rtos_lock_release(&__rtos_malloc_lock);

	return status;
}
