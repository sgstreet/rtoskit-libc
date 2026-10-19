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

#include "internal/malloc.h"

/* Both allocators align payloads to two words (PLAN D31, D32), above alignof(max_align_t) on x86_64 */
static constexpr size_t heap_align = 2 * sizeof(size_t);

/*
 * The allocator (P1.04), built once per variant (malloc-nano, malloc-tlsf): alignment, zero sizes, overflow, exhaustion and
 * recovery, realloc in place and by moving, aligned_alloc, strdup/strndup,
 * and a randomised operation sequence with the heap consistency checker
 * after every step. Vectors from docs/research/malloc.md.
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
