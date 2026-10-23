/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * malloc.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtos/rtos-port.h>

#include "internal/malloc.h"

/* Every allocator aligns payloads to two words (PLAN D31, D32, D54), above alignof(max_align_t) on x86_64 */
static constexpr size_t heap_align = 2 * sizeof(size_t);

/*
 * The allocator (P1.04), built once per variant (malloc-nano, malloc-tlsf, malloc-tiny): alignment, zero sizes, overflow, exhaustion and
 * recovery, realloc in place and by moving, aligned_alloc, strdup/strndup,
 * a randomised operation sequence with the heap consistency checker
 * after every step, and growth through __rtos_sbrk when something else
 * moves the break (PLAN D103). Vectors from docs/research/malloc.md.
 */

static int failures;

static void fail(int line, const char *what)
{
	char message[256];

	snprintf(message, sizeof(message), "fail: line %d: %s\n", line, what);
	fputs(message, stdout);
	++failures;
}

#define CHECK(condition) ((condition) ? (void)0 : fail(__LINE__, #condition))

static bool aligned(const void *p, size_t alignment)
{
	return ((uintptr_t)p & (alignment - 1)) == 0;
}

static void fill(unsigned char *p, size_t n, unsigned char seed)
{
	for (size_t i = 0; i < n; ++i)
		p[i] = (unsigned char)(seed + i);
}

static bool filled(const unsigned char *p, size_t n, unsigned char seed)
{
	for (size_t i = 0; i < n; ++i)
		if (p[i] != (unsigned char)(seed + i))
			return false;

	return true;
}

/*
 * The growth helper's contract on a heap record of its own (D103): in place
 * when nothing moved the break, otherwise a new segment at least a minimum
 * chunk above the old end that holds the stride, for every
 * misalignment of the foreign break and for both minimum strides in use.
 */
static void test_grow_geometry(void)
{
	static const size_t min_strides[] = { heap_align, 2 * heap_align };

	for (size_t m = 0; m < sizeof(min_strides) / sizeof(min_strides[0]); ++m) {
		size_t min_stride = min_strides[m];
		struct __libc_malloc_heap heap = {};
		char *bridge = nullptr;
		char *chunk = nullptr;

		CHECK(__libc_malloc_grow(&heap, 4 * heap_align, min_stride, &chunk, &bridge));
		CHECK(bridge == chunk && heap.first == chunk && aligned(chunk, heap_align) && heap.end - chunk >= (ptrdiff_t)(4 * heap_align));

		for (size_t offset = 0; offset < 2 * heap_align; ++offset) {
			for (size_t stride = heap_align; stride <= 4 * heap_align; stride += heap_align) {
				char *end = heap.end;
				char *brk = heap.brk;

				/* In place: the new memory starts at the old end and adds at least stride */
				CHECK(__libc_malloc_grow(&heap, stride, min_stride, &chunk, &bridge));
				CHECK(chunk == end && bridge == chunk && aligned(heap.end, heap_align) && heap.end - end >= (ptrdiff_t)stride && heap.end <= heap.brk && heap.brk > brk);

				/* Moved: a bridge of at least min_stride, then at least a whole chunk */
				end = heap.end;
				CHECK(offset == 0 || __rtos_sbrk((ptrdiff_t)offset) != (void *)-1);
				CHECK(__libc_malloc_grow(&heap, stride, min_stride, &chunk, &bridge));
				if (offset == 0)
					continue;
				CHECK(bridge == end && chunk - end >= (ptrdiff_t)min_stride && aligned(chunk, heap_align) && aligned(heap.end, heap_align));
				CHECK(heap.end - chunk >= (ptrdiff_t)stride && heap.end <= heap.brk && heap.brk - heap.end < (ptrdiff_t)heap_align);
			}
		}
	}

	/* Requests above the bound fail before they reach __rtos_sbrk, so every increment is positive */
	errno = 0;
	CHECK(malloc((size_t)__LIBC_MALLOC_MAX + 1) == nullptr && errno == ENOMEM);
	errno = 0;
	CHECK(aligned_alloc((size_t)1 << (sizeof(size_t) * 8 - 2), 16) == nullptr && errno == ENOMEM);
}

/*
 * Move the break behind the allocator's back between allocations (D103), so
 * the heap continues in a new segment over a bridge: with a big request,
 * and with a free top chunk followed by a slightly larger request, for
 * breaks moved by one byte, an alignment unit and an odd amount. Runs on
 * the fresh heap, before the capacity is measured.
 */
static void test_foreign_break(void)
{
	static const size_t extra[] = { 1, 2 * sizeof(size_t), 3 };
	unsigned char *blocks[4];
	char *foreign;

	blocks[0] = malloc(1000);
	CHECK(blocks[0] != nullptr);
	fill(blocks[0], 1000, 1);

	/* An unaligned foreign piece, then a request that needs a new segment */
	foreign = __rtos_sbrk(4096 + 3);
	CHECK(foreign != (char *)-1);
	blocks[1] = malloc(100000);
	CHECK(blocks[1] != nullptr && aligned(blocks[1], heap_align) && (char *)blocks[1] > foreign + 4096);
	fill(blocks[1], 100000, 2);
	CHECK(__libc_malloc_check());

	/* A free top chunk and a slightly larger request: the new segment lies beyond the foreign bytes */
	for (size_t i = 0; i < sizeof(extra) / sizeof(extra[0]); ++i) {
		unsigned char *top = malloc(2000);

		CHECK(top != nullptr);
		free(top);
		CHECK(__rtos_sbrk((ptrdiff_t)extra[i]) != (void *)-1);
		top = malloc(2000 + 8 * sizeof(size_t) * i + 64);
		CHECK(top != nullptr && aligned(top, heap_align));
		fill(top, 2000, 3);
		CHECK(filled(top, 2000, 3));
		CHECK(__libc_malloc_check());
		free(top);
		CHECK(__libc_malloc_check());
	}

	/* Bridges never merge: freeing around them keeps the heap consistent and the payloads intact */
	blocks[2] = malloc(300);
	blocks[3] = realloc(blocks[0], 50000);
	CHECK(blocks[2] != nullptr && blocks[3] != nullptr && filled(blocks[3], 1000, 1) && filled(blocks[1], 100000, 2));
	free(blocks[1]);
	CHECK(__libc_malloc_check());
	free(blocks[3]);
	free(blocks[2]);
	CHECK(__libc_malloc_check());

	/* Everything is free again: no allocation, however it walks or merges, may land in the foreign bytes */
	{
		static void *again[64];

		for (size_t i = 0; i < sizeof(again) / sizeof(again[0]); ++i) {
			again[i] = malloc(16 + 64 * i);
			CHECK(again[i] != nullptr && ((char *)again[i] + 16 + 64 * i <= foreign || (char *)again[i] >= foreign + 4096 + 3));
		}
		for (size_t i = 0; i < sizeof(again) / sizeof(again[0]); ++i)
			free(again[i]);
		CHECK(__libc_malloc_check());
	}
}

/* The largest single allocation that succeeds right now, found by bisection */
static size_t largest_allocation(void)
{
	size_t low = 0;
	size_t high = (size_t)1 << 30;

	while (low < high) {
		size_t middle = low + (high - low + 1) / 2;
		void *p = malloc(middle);

		if (p != nullptr) {
			free(p);
			low = middle;
		} else {
			high = middle - 1;
		}
	}

	return low;
}

/* Measured on the fresh heap; a fully freed heap must offer it again, or memory leaked */
static size_t capacity;

static void check_all_free(int line)
{
	void *p = malloc(capacity);

	if (p == nullptr)
		fail(line, "the freed heap no longer holds its first largest allocation");
	free(p);
}

static void test_basics(void)
{
	void *zero[3];
	unsigned char *p;
	int *array;

	/* Unique pointers for zero sizes (J.3.12) */
	for (int i = 0; i < 3; ++i)
		zero[i] = malloc(0);
	CHECK(zero[0] != nullptr && zero[1] != nullptr && zero[2] != nullptr);
	CHECK(zero[0] != zero[1] && zero[1] != zero[2] && zero[0] != zero[2]);
	for (int i = 0; i < 3; ++i)
		free(zero[i]);

	free(nullptr);
	free_sized(nullptr, 10);
	free_aligned_sized(nullptr, 64, 10);

	for (size_t size = 1; size < 300; size += 7) {
		p = malloc(size);
		CHECK(p != nullptr && aligned(p, alignof(max_align_t)) && aligned(p, heap_align));
		fill(p, size, (unsigned char)size);
		CHECK(filled(p, size, (unsigned char)size));
		free_sized(p, size);
	}

	array = calloc(100, sizeof(*array));
	CHECK(array != nullptr);
	for (int i = 0; array != nullptr && i < 100; ++i)
		CHECK(array[i] == 0);
	free(array);

	/* Overflow: nothing is allocated and errno says why */
	errno = 0;
	CHECK(calloc(SIZE_MAX / 2, 3) == nullptr && errno == ENOMEM);
	errno = 0;
	CHECK(malloc(SIZE_MAX) == nullptr && errno == ENOMEM);
	errno = 0;
	CHECK(malloc(SIZE_MAX - 3 * sizeof(size_t)) == nullptr && errno == ENOMEM);
	errno = 0;
	CHECK(aligned_alloc(64, SIZE_MAX - 8) == nullptr && errno == ENOMEM);
	CHECK(__libc_malloc_check());
}

static void test_realloc(void)
{
	unsigned char *p = realloc(nullptr, 40);
	unsigned char *q;
	unsigned char *blocker;

	CHECK(p != nullptr);
	fill(p, 40, 3);

	/* Shrink in place keeps the contents */
	q = realloc(p, 10);
	CHECK(q == p && filled(q, 10, 3));

	/* Grow in place into the free space just released */
	q = realloc(p, 40);
	CHECK(q == p && filled(q, 10, 3));

	/* Grow by moving when the successor is in use */
	blocker = malloc(16);
	fill(q, 40, 9);
	p = realloc(q, 4000);
	CHECK(p != nullptr && filled(p, 40, 9));
	CHECK(__libc_malloc_check());

	/* Size zero shrinks to a minimum block (J.3.12) */
	q = realloc(p, 0);
	CHECK(q == p);
	free(q);
	free(blocker);

	/* A failed realloc leaves the old object alone */
	p = malloc(32);
	fill(p, 32, 5);
	errno = 0;
	CHECK(realloc(p, SIZE_MAX) == nullptr && errno == ENOMEM);
	CHECK(filled(p, 32, 5));
	free(p);
	CHECK(__libc_malloc_check());
}

static void test_aligned_alloc(void)
{
	for (size_t alignment = 1; alignment <= 4096; alignment *= 2) {
		for (size_t size = 0; size < 200; size += 33) {
			unsigned char *p = aligned_alloc(alignment, size);

			CHECK(p != nullptr && aligned(p, alignment) && aligned(p, heap_align));
			fill(p, size, 1);
			CHECK(__libc_malloc_check());
			free_aligned_sized(p, alignment, size);
		}
	}

	errno = 0;
	CHECK(aligned_alloc(24, 10) == nullptr && errno == EINVAL);
	CHECK(aligned_alloc(0, 10) == nullptr);
	CHECK(__libc_malloc_check());
}

static void test_strdup(void)
{
	char unterminated[3] = { 'x', 'y', 'z' };
	char *copy = strdup("hello");

	CHECK(copy != nullptr && strcmp(copy, "hello") == 0);
	free(copy);

	copy = strndup("hello", 3);
	CHECK(copy != nullptr && strcmp(copy, "hel") == 0);
	free(copy);

	copy = strndup("hi", 10);
	CHECK(copy != nullptr && strcmp(copy, "hi") == 0);
	free(copy);

	copy = strndup(unterminated, 3);
	CHECK(copy != nullptr && strcmp(copy, "xyz") == 0);
	free(copy);
}

/* Fill the heap with large then small blocks, free it all, and get the space back */
static void test_exhaustion(void)
{
	static void *blocks[100000];
	size_t count = 0;
	size_t total = 0;
	void *big;

	for (size_t size = (size_t)1 << 24; size >= 16; size /= 2) {
		void *p;

		while (count < sizeof(blocks) / sizeof(blocks[0]) && (p = malloc(size)) != nullptr) {
			blocks[count++] = p;
			total += size;
		}
	}
	CHECK(count > 0 && count < sizeof(blocks) / sizeof(blocks[0]));

	errno = 0;
	CHECK(malloc(64) == nullptr && errno == ENOMEM);
	CHECK(__libc_malloc_check());

	/* Free in an interleaved order so both merge directions are exercised */
	for (size_t i = 0; i < count; i += 2)
		free(blocks[i]);
	CHECK(__libc_malloc_check());
	for (size_t i = 1; i < count; i += 2)
		free(blocks[i]);
	CHECK(__libc_malloc_check());

	/* Everything merged back: half of what fitted before fits in one block */
	big = malloc(total / 2);
	CHECK(big != nullptr);
	free(big);
	CHECK(__libc_malloc_check());
	check_all_free(__LINE__);
}

/* Random mixed operations with checked payloads and a full heap check after each */
static void test_fuzz(void)
{
	enum { SLOTS = 64,
		   STEPS = 20000 };
	static struct {
		unsigned char *p;
		size_t size;
		unsigned char seed;
	} slots[SLOTS];
	uint32_t state = 0x12345678;

	for (int step = 0; step < STEPS && failures == 0; ++step) {
		unsigned int slot;
		unsigned int action;
		size_t size;

		state = state * 1664525 + 1013904223;
		slot = (state >> 8) % SLOTS;
		action = (state >> 16) % 6;
		size = (state >> 20) % 512;
		if ((state & 0xff) == 0)
			size *= 97;

		if (slots[slot].p != nullptr && !filled(slots[slot].p, slots[slot].size, slots[slot].seed))
			fail(__LINE__, "payload corrupted");

		if (slots[slot].p != nullptr && action < 3) {
			free(slots[slot].p);
			slots[slot].p = nullptr;
		} else if (slots[slot].p != nullptr) {
			unsigned char *q = realloc(slots[slot].p, size);
			size_t keep = size < slots[slot].size ? size : slots[slot].size;

			if (q == nullptr)
				continue;
			if (!filled(q, keep, slots[slot].seed))
				fail(__LINE__, "realloc lost the contents");
			slots[slot].p = q;
			slots[slot].size = size;
			fill(q, size, slots[slot].seed);
		} else {
			unsigned char *q;

			if (action == 0)
				q = calloc(1, size);
			else if (action == 1)
				q = aligned_alloc((size_t)16 << (state % 6), size);
			else
				q = malloc(size);
			if (q == nullptr)
				continue;
			if (action == 0)
				for (size_t i = 0; i < size; ++i)
					if (q[i] != 0)
						fail(__LINE__, "calloc memory not zero");
			slots[slot].p = q;
			slots[slot].size = size;
			slots[slot].seed = (unsigned char)state;
			fill(q, size, slots[slot].seed);
		}

		if (!__libc_malloc_check()) {
			char message[64];

			snprintf(message, sizeof(message), "heap check failed at step %d", step);
			fail(__LINE__, message);
		}
	}

	for (unsigned int i = 0; i < SLOTS; ++i)
		free(slots[i].p);
	CHECK(__libc_malloc_check());
	check_all_free(__LINE__);
}

int main(void)
{
	test_grow_geometry();
	test_foreign_break();
	capacity = largest_allocation();
	CHECK(capacity > 0);

	test_basics();
	check_all_free(__LINE__);
	test_realloc();
	test_aligned_alloc();
	test_strdup();
	check_all_free(__LINE__);
	test_fuzz();
	test_exhaustion();

	if (failures == 0)
		puts("pass: malloc");

	return failures == 0 ? 0 : 1;
}
