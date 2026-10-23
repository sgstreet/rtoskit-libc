/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * malloc.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_MALLOC_H
#define _INTERNAL_MALLOC_H

/*
 * Services every allocator variant (libc/malloc/<variant>, chosen with
 * LIBC_MALLOC, PLAN D32) provides beyond the standard functions.
 */

#include <stddef.h>
#include <stdint.h>
#include <rtos/rtos-port.h>

/* Verifies every invariant of the heap; takes the lock itself (for tests) */
bool __libc_malloc_check(void);

/*
 * The extent of a heap grown with __rtos_sbrk (PLAN D103), shared by the
 * allocators. Chunks tile first..end: end is the payload address of the end
 * marker, whose header word is the heap's last word, and brk is the break as
 * the heap last left it (end is brk rounded down to the alignment). first is
 * null until the first growth.
 */
/*
 * The largest request any allocator accepts (D103): a stride, plus an
 * alignment of up to the same size, TLSF's class rounding and the growth
 * slack, stays below PTRDIFF_MAX, so every __rtos_sbrk increment is positive.
 */
#define __LIBC_MALLOC_MAX (PTRDIFF_MAX / 4)

struct __libc_malloc_heap {
	char *first;
	char *end;
	char *brk;
};

/*
 * Grow the heap by at least stride bytes, lock held (docs/research/malloc.md,
 * "Growing with sbrk"). If the break is still at brk, the new memory extends
 * the heap in place and *chunk is the old end. Otherwise a new segment
 * starts above the old one, and *chunk is its first chunk, at least
 * min_stride above the old end. Either way *chunk..end holds at least
 * stride bytes. *bridge is the old end marker, which the caller turns into
 * a used bridge chunk reaching *chunk, or *chunk itself when there is no
 * bridge (in place, or the first growth): then the bridge header the caller
 * writes is overwritten by the chunk's. The caller then makes *chunk..end a
 * free chunk and writes the new end marker. In place the break moves by
 * exactly stride, so steady growth leaves no slack chunks behind (tiny would
 * walk them); a new segment takes a second increment for its slack (if
 * something moves the break between the two, that memory is lost and the
 * growth fails). Payloads are aligned to 2 * sizeof(size_t); stride comes
 * from a request of at most __LIBC_MALLOC_MAX. Returns false when
 * __rtos_sbrk has no more memory.
 * Inline, so each allocator folds its own min_stride.
 */
static inline bool __libc_malloc_grow(struct __libc_malloc_heap *heap, size_t stride, size_t min_stride, char **chunk, char **bridge)
{
	constexpr uintptr_t align = 2 * sizeof(size_t);
	/* A new segment also needs a bridge's offset and the alignment of both ends */
	size_t slack = min_stride + 2 * align;
	size_t increment = heap->brk != nullptr ? stride : stride + slack;
	char *p = __rtos_sbrk((ptrdiff_t)increment);

	if (p == (char *)-1)
		return false;

	/* Moved by someone else: the slack follows, from where this call left the break */
	if (heap->brk != nullptr && p != heap->brk) {
		if (__rtos_sbrk((ptrdiff_t)slack) != p + increment)
			return false;
		increment += slack;
	}

	/* brk is null until the first growth, so it only matches a break nothing else moved */
	*chunk = heap->end;
	*bridge = heap->end;
	if (p != heap->brk) {
		/* Another user moved the break (never below the heap, rtos-port.h): bridge over at least a minimum chunk */
		*chunk = (char *)(((uintptr_t)p + min_stride + align - 1) & ~(align - 1));
		if (heap->first == nullptr)
			heap->first = *bridge = *chunk;
	}

	heap->brk = p + increment;
	heap->end = (char *)((uintptr_t)heap->brk & ~(align - 1));

	return true;
}

#endif /* _INTERNAL_MALLOC_H */
