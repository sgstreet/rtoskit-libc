/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * nano.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _NANO_H
#define _NANO_H

#include <stddef.h>
#include <stdint.h>

/*
 * The small allocator, the default (LIBC_MALLOC=nano; docs/research/malloc.md,
 * PLAN D32): first fit over one free list kept in address order, merging
 * neighbours on free.
 *
 * A chunk is named by its payload address p. The word before p holds its
 * stride, the distance to the next chunk's payload, so a used chunk costs one
 * word. A free chunk keeps the link to the next free chunk at p.
 */

#define __LIBC_NANO_WORD sizeof(size_t)
#define __LIBC_NANO_ALIGN (2 * sizeof(size_t))
#define __LIBC_NANO_MIN_STRIDE __LIBC_NANO_ALIGN

static_assert(__LIBC_NANO_ALIGN >= alignof(max_align_t), "payloads must suit every fundamental type");

struct __libc_nano_chunk {
	struct __libc_nano_chunk *next;
};

/* Kept at the start of each region so the checker can walk it */
struct __libc_nano_region {
	struct __libc_nano_region *next;
	void *first;
	void *end;
};

/* Protected by __rtos_malloc_lock */
struct __libc_nano_heap {
	struct __libc_nano_chunk *free_list;
	struct __libc_nano_region *regions;
	bool initialised;
};

extern struct __libc_nano_heap __libc_nano_heap;

static inline size_t *__libc_nano_header(void *p)
{
	return (size_t *)((char *)p - __LIBC_NANO_WORD);
}

static inline bool __libc_nano_below(const void *a, const void *b)
{
	return (uintptr_t)a < (uintptr_t)b;
}

/* The stride holding size bytes, or 0 when it cannot be represented */
static inline size_t __libc_nano_request(size_t size)
{
	if (size > SIZE_MAX - __LIBC_NANO_WORD - (__LIBC_NANO_ALIGN - 1))
		return 0;

	return (size + __LIBC_NANO_WORD + __LIBC_NANO_ALIGN - 1) & ~(__LIBC_NANO_ALIGN - 1);
}

/* Lock held by the callers of every function below */
void __libc_nano_init(void);
void *__libc_nano_allocate(size_t stride);
void __libc_nano_release(void *p);
void __libc_nano_trim(void *p, size_t stride);

#endif /* _NANO_H */
