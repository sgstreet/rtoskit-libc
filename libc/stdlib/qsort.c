/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * qsort.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "internal/word.h"

/*
 * C23 7.24.5.2 with heapsort (docs/research/stdlib-part2.md): O(n log n) in
 * the worst case, no recursion and no extra memory. Not stable, which the
 * standard allows.
 */

struct sort {
	unsigned char *base;
	size_t size;
	int (*compar)(const void *, const void *);
	bool words;
};

static unsigned char *element(const struct sort *sort, size_t i)
{
	return sort->base + i * sort->size;
}

static void swap(const struct sort *sort, size_t i, size_t j)
{
	unsigned char *a = element(sort, i);
	unsigned char *b = element(sort, j);

	if (sort->words) {
		/* Elements may have any type, so words move through the aliasing-safe helpers */
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

/* Move element root down until the max-heap property holds within [0, count) */
static void sift_down(const struct sort *sort, size_t root, size_t count)
{
	while (true) {
		size_t child = 2 * root + 1;

		if (child >= count)
			return;
		if (child + 1 < count && sort->compar(element(sort, child), element(sort, child + 1)) < 0)
			++child;
		if (sort->compar(element(sort, root), element(sort, child)) >= 0)
			return;

		swap(sort, root, child);
		root = child;
	}
}

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *))
{
	struct sort sort = {
		.base = base,
		.size = size,
		.compar = compar,
		.words = size % __LIBC_WORD_SIZE == 0 && __libc_word_aligned(base),
	};

	if (nmemb < 2 || size == 0)
		return;

	/* Floyd: build the heap bottom up, then repeatedly move the maximum to the end */
	for (size_t i = nmemb / 2; i > 0; --i)
		sift_down(&sort, i - 1, nmemb);
	for (size_t end = nmemb - 1; end > 0; --end) {
		swap(&sort, 0, end);
		sift_down(&sort, 0, end);
	}
}
