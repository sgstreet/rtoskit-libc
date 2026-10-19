/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tlsf.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _TLSF_H
#define _TLSF_H

#include <limits.h>
#include <stdbit.h>
#include <stddef.h>
#include <stdint.h>

/*
 * TLSF allocator, selected with LIBC_MALLOC=tlsf (docs/research/malloc.md, PLAN D31, D32).
 *
 * A block is named by its payload address p. The word before p holds the
 * stride, the distance to the next block's payload, with two flag bits. A free
 * block keeps its free-list links at p and a pointer to itself in the word
 * just before the next block's header, so a used block costs one word.
 */

#define __LIBC_TLSF_WORD sizeof(size_t)
#define __LIBC_TLSF_ALIGN (2 * sizeof(size_t))
#define __LIBC_TLSF_MIN_STRIDE (4 * sizeof(size_t))
#define __LIBC_TLSF_SL_LOG2 4U
#define __LIBC_TLSF_SL_COUNT (1U << __LIBC_TLSF_SL_LOG2)
#define __LIBC_TLSF_SMALL (__LIBC_TLSF_SL_COUNT * __LIBC_TLSF_ALIGN)
#define __LIBC_TLSF_FL_MAX (sizeof(size_t) * CHAR_BIT)
#define __LIBC_TLSF_MAX_REGIONS 8

/* Flags in the low bits of the stride word */
#define __LIBC_TLSF_FREE 1U
#define __LIBC_TLSF_PREV_FREE 2U
#define __LIBC_TLSF_FLAGS 3U

static_assert(__LIBC_TLSF_ALIGN >= alignof(max_align_t), "payloads must suit every fundamental type");
static_assert(sizeof(void *) == sizeof(size_t), "back pointers share the stride word's size");
static_assert(__LIBC_TLSF_MIN_STRIDE % __LIBC_TLSF_ALIGN == 0, "strides are multiples of the alignment");

struct __libc_free_block {
	struct __libc_free_block *next;
	struct __libc_free_block *prev;
};

struct __libc_tlsf_region {
	void *first; /* payload of the first block */
	void *end; /* payload address of the zero-stride sentinel */
};

/* The control block, carved from the start of the largest region */
struct __libc_tlsf {
	unsigned int fl_count;
	unsigned int region_count;
	struct __libc_tlsf_region regions[__LIBC_TLSF_MAX_REGIONS];
	size_t fl_bitmap;
	uint32_t sl_bitmap[__LIBC_TLSF_FL_MAX];
	struct __libc_free_block *lists[][__LIBC_TLSF_SL_COUNT];
};

static inline size_t *__libc_tlsf_header(void *p)
{
	return (size_t *)((char *)p - __LIBC_TLSF_WORD);
}

static inline size_t __libc_tlsf_stride(void *p)
{
	return *__libc_tlsf_header(p) & ~(size_t)__LIBC_TLSF_FLAGS;
}

static inline bool __libc_tlsf_is_free(void *p)
{
	return (*__libc_tlsf_header(p) & __LIBC_TLSF_FREE) != 0;
}

static inline bool __libc_tlsf_prev_is_free(void *p)
{
	return (*__libc_tlsf_header(p) & __LIBC_TLSF_PREV_FREE) != 0;
}

static inline void *__libc_tlsf_next(void *p)
{
	return (char *)p + __libc_tlsf_stride(p);
}

/* The back pointer of the block ending at next; valid when that block is free */
static inline void **__libc_tlsf_back_pointer(void *next)
{
	return (void **)((char *)next - 2 * __LIBC_TLSF_WORD);
}

static inline unsigned int __libc_tlsf_log2(size_t x)
{
	return stdc_bit_width(x) - 1;
}

/* The lists a stride belongs to: exact classes below SMALL, 16 per power of two above */
static inline void __libc_tlsf_mapping(size_t stride, unsigned int *fl, unsigned int *sl)
{
	unsigned int small_log2 = __libc_tlsf_log2(__LIBC_TLSF_SMALL);
	unsigned int f;

	if (stride < __LIBC_TLSF_SMALL) {
		*fl = 0;
		*sl = stride / __LIBC_TLSF_ALIGN;
		return;
	}

	f = __libc_tlsf_log2(stride);
	*fl = f - small_log2 + 1;
	*sl = (stride >> (f - __LIBC_TLSF_SL_LOG2)) - __LIBC_TLSF_SL_COUNT;
}

/* The stride holding size bytes, or 0 when it cannot be represented */
static inline size_t __libc_tlsf_request(size_t size)
{
	size_t stride;

	if (size > SIZE_MAX - __LIBC_TLSF_WORD - (__LIBC_TLSF_ALIGN - 1))
		return 0;

	stride = (size + __LIBC_TLSF_WORD + __LIBC_TLSF_ALIGN - 1) & ~(__LIBC_TLSF_ALIGN - 1);
	return stride < __LIBC_TLSF_MIN_STRIDE ? __LIBC_TLSF_MIN_STRIDE : stride;
}

/* Lock held by the callers of every function below */
struct __libc_tlsf *__libc_tlsf_get(void);
void __libc_tlsf_insert(struct __libc_tlsf *heap, void *p);
void __libc_tlsf_remove(struct __libc_tlsf *heap, void *p);
void *__libc_tlsf_allocate(struct __libc_tlsf *heap, size_t stride);
void __libc_tlsf_trim(struct __libc_tlsf *heap, void *p, size_t stride);
void __libc_tlsf_release(struct __libc_tlsf *heap, void *p);

#endif /* _TLSF_H */
