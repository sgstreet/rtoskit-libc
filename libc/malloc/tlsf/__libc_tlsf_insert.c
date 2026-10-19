/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_tlsf_insert.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "internal/malloc.h"
#include "tlsf.h"

/* Push a free block on the head of its list */
void __libc_tlsf_insert(struct __libc_tlsf *heap, void *p)
{
	struct __libc_free_block *block = p;
	unsigned int fl;
	unsigned int sl;

	__libc_tlsf_mapping(__libc_tlsf_stride(p), &fl, &sl);

	block->prev = nullptr;
	block->next = heap->lists[fl][sl];
	if (block->next != nullptr)
		block->next->prev = block;
	heap->lists[fl][sl] = block;

	heap->sl_bitmap[fl] |= 1U << sl;
	heap->fl_bitmap |= (size_t)1 << fl;
}
