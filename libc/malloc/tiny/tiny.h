/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tiny.h
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _TINY_H
#define _TINY_H

#include <stddef.h>
#include <stdint.h>

/*
 * The tiny allocator (LIBC_MALLOC=tiny, libc-tinymalloc.a; docs/research/
 * malloc-tiny.md, PLAN D54): an implicit free list. A chunk is named by its
 * payload address p; the word before p holds its stride, the distance to
 * the next chunk's payload, with bit 0 set while the chunk is in use. The
 * chunks tile the port's first heap region (the only one used), and a used
 * sentinel chunk of stride 0 ends it, so the heap is found by walking from
 * one pointer and merging stops at the sentinel by itself. free only clears the bit; allocation walks first fit,
 * merging the free chunks it passes. Smaller than nano, with a slower malloc.
 */

#define __LIBC_TINY_WORD sizeof(size_t)
#define __LIBC_TINY_ALIGN (2 * sizeof(size_t))
#define __LIBC_TINY_MIN_STRIDE __LIBC_TINY_ALIGN
#define __LIBC_TINY_USED ((size_t)1)

static_assert(__LIBC_TINY_ALIGN >= alignof(max_align_t), "payloads must suit every fundamental type");

/* Protected by __rtos_malloc_lock; first is null until the heap is set up */
struct __libc_tiny_heap {
	char *first;
	bool initialised;
};

extern struct __libc_tiny_heap __libc_tiny_heap;

static inline size_t *__libc_tiny_header(void *p)
{
	return (size_t *)((char *)p - __LIBC_TINY_WORD);
}

static inline size_t __libc_tiny_stride(void *p)
{
	return *__libc_tiny_header(p) & ~__LIBC_TINY_USED;
}

/* The stride holding size bytes, or 0 when it cannot be represented */
static inline size_t __libc_tiny_request(size_t size)
{
	if (size > SIZE_MAX - __LIBC_TINY_WORD - (__LIBC_TINY_ALIGN - 1))
		return 0;

	return (size + __LIBC_TINY_WORD + __LIBC_TINY_ALIGN - 1) & ~(__LIBC_TINY_ALIGN - 1);
}

/* Lock held by the callers of every function below */
void __libc_tiny_init(void);
void *__libc_tiny_allocate(size_t stride);
void __libc_tiny_merge(void *p);
void __libc_tiny_trim(void *p, size_t stride);

#endif /* _TINY_H */
