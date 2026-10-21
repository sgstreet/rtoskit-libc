/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * qsort.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "internal/word.h"

/*
 * C23 7.24.5.2, the speed flavour (PLAN D90, docs/research/qsort-speed.md):
 * an input in order, all equal or strictly reversed is settled in one pass;
 * otherwise introsort. Ranges above SMALL elements are partitioned around a median of
 * three (a ninther above NINTHER) with Hoare's exchanges, both scans
 * stopping on keys equal to the pivot so equal keys split evenly; smaller
 * ranges are finished by insertion sort; a range that exhausts its depth
 * budget of 2 log2 n is heapsorted, which bounds the worst case at
 * O(n log n). No recursion: the larger part of each partition waits on a
 * stack while the smaller is sorted, so the stack never holds more than
 * log2 n ranges. Not stable, which the standard allows.
 */

#define SMALL 8
#define NINTHER 64

struct sort {
	unsigned char *base;
	size_t size;
	int (*compar)(const void *, const void *);
	bool words;
	bool word;
};

struct range {
	size_t first;
	size_t count;
	unsigned int depth;
};

static unsigned char *element(const struct sort *sort, size_t i)
{
	return sort->base + i * sort->size;
}

static void swap(const struct sort *sort, unsigned char *a, unsigned char *b)
{
	/* Elements may have any type, so words move through the aliasing-safe helpers */
	if (sort->word) {
		uintptr_t t = __libc_word_load(a);

		__libc_word_store(a, __libc_word_load(b));
		__libc_word_store(b, t);
		return;
	}
	if (sort->words) {
		for (size_t k = 0; k < sort->size; k += __LIBC_WORD_SIZE) {
			uintptr_t t = __libc_word_load(a + k);

			__libc_word_store(a + k, __libc_word_load(b + k));
			__libc_word_store(b + k, t);
		}
		return;
	}

	for (size_t k = 0; k < sort->size; ++k) {
		unsigned char t = a[k];

		a[k] = b[k];
		b[k] = t;
	}
}

/* The median of the elements at a, b and c */
static unsigned char *median(const struct sort *sort, unsigned char *a, unsigned char *b, unsigned char *c)
{
	if (sort->compar(a, b) < 0) {
		if (sort->compar(b, c) < 0)
			return b;
		return sort->compar(a, c) < 0 ? c : a;
	}
	if (sort->compar(a, c) < 0)
		return a;
	return sort->compar(b, c) < 0 ? c : b;
}

static void insertion_sort(const struct sort *sort, unsigned char *first, size_t count)
{
	unsigned char *end = first + count * sort->size;

	for (unsigned char *i = first + sort->size; i < end; i += sort->size)
		for (unsigned char *j = i; j > first && sort->compar(j - sort->size, j) > 0; j -= sort->size)
			swap(sort, j - sort->size, j);
}

/* Move element root down until the max-heap property holds within the count elements from first */
static void sift_down(const struct sort *sort, unsigned char *first, size_t root, size_t count)
{
	while (true) {
		size_t child = 2 * root + 1;
		unsigned char *larger;

		if (child >= count)
			return;
		larger = first + child * sort->size;
		if (child + 1 < count && sort->compar(larger, larger + sort->size) < 0) {
			larger += sort->size;
			++child;
		}
		if (sort->compar(first + root * sort->size, larger) >= 0)
			return;

		swap(sort, first + root * sort->size, larger);
		root = child;
	}
}

/* Floyd's heapsort of the count elements from first: the fallback that bounds the worst case */
static void heap_sort(const struct sort *sort, unsigned char *first, size_t count)
{
	for (size_t i = count / 2; i > 0; --i)
		sift_down(sort, first, i - 1, count);
	for (size_t end = count - 1; end > 0; --end) {
		swap(sort, first, first + end * sort->size);
		sift_down(sort, first, 0, end);
	}
}

/*
 * An input already in order, or all equal, costs n - 1 comparisons, and one
 * in strictly descending order n - 1 and a reversal: true when that sorted
 * it. Other inputs stop at the first pair out of the run, after about two
 * comparisons.
 */
static bool presorted(const struct sort *sort, size_t count)
{
	unsigned char *last = element(sort, count - 1);
	unsigned char *i = sort->base;

	if (sort->compar(i, i + sort->size) > 0) {
		while (i < last && sort->compar(i, i + sort->size) > 0)
			i += sort->size;
		if (i < last)
			return false;
		for (unsigned char *a = sort->base, *b = last; a < b; a += sort->size, b -= sort->size)
			swap(sort, a, b);
		return true;
	}

	while (i < last && sort->compar(i, i + sort->size) <= 0)
		i += sort->size;
	return i == last;
}

/* Partition the count (> SMALL) elements from first; the pivot's final index from first */
static size_t partition(const struct sort *sort, unsigned char *first, size_t count)
{
	size_t size = sort->size;
	unsigned char *last = first + (count - 1) * size;
	unsigned char *middle = first + count / 2 * size;
	unsigned char *i = first + size;
	unsigned char *j = last;
	unsigned char *pivot;

	if (count > NINTHER) {
		size_t step = count / 8 * size;

		pivot = median(sort, median(sort, first, first + step, first + 2 * step), median(sort, middle - step, middle, middle + step), median(sort, last - 2 * step, last - step, last));
	} else {
		pivot = median(sort, first, middle, last);
	}
	swap(sort, first, pivot);

	/* [first + 1, i) holds keys <= the pivot and (j, last] keys >= it; both scans stop on equal keys */
	while (true) {
		while (i <= j && sort->compar(i, first) < 0)
			i += size;
		while (i <= j && sort->compar(j, first) > 0)
			j -= size;
		if (i >= j)
			break;
		swap(sort, i, j);
		i += size;
		j -= size;
	}
	swap(sort, first, j);

	return (size_t)(j - first) / size;
}

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *))
{
	struct sort sort = {
		.base = base,
		.size = size,
		.compar = compar,
		.words = size % __LIBC_WORD_SIZE == 0 && __libc_word_aligned(base),
		.word = size == __LIBC_WORD_SIZE && __libc_word_aligned(base),
	};
	/* The smaller part goes first, so at most log2 nmemb ranges wait */
	struct range stack[sizeof(size_t) * CHAR_BIT];
	size_t waiting = 0;
	struct range current = { .first = 0, .count = nmemb, .depth = 0 };

	if (nmemb < 2 || size == 0 || presorted(&sort, nmemb))
		return;

	for (size_t n = nmemb; n > 1; n >>= 1)
		current.depth += 2;

	while (true) {
		while (current.count > SMALL) {
			size_t pivot;
			struct range left;
			struct range right;

			if (current.depth == 0) {
				heap_sort(&sort, element(&sort, current.first), current.count);
				current.count = 0;
				break;
			}

			pivot = current.first + partition(&sort, element(&sort, current.first), current.count);
			left = (struct range){ .first = current.first, .count = pivot - current.first, .depth = current.depth - 1 };
			right = (struct range){ .first = pivot + 1, .count = current.first + current.count - pivot - 1, .depth = current.depth - 1 };
			if (left.count < right.count) {
				stack[waiting++] = right;
				current = left;
			} else {
				stack[waiting++] = left;
				current = right;
			}
		}
		insertion_sort(&sort, element(&sort, current.first), current.count);

		if (waiting == 0)
			return;
		current = stack[--waiting];
	}
}
