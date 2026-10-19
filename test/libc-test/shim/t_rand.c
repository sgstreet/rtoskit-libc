/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * t_rand.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <stdint.h>

#include "test.h"

/*
 * Replacements for libc-test's t_rand* helpers (P1.05): a 64-bit LCG with
 * rejection sampling for uniform ranges and a Fisher-Yates shuffle. Only the
 * interface matches libc-test; the sequences need not.
 */
static uint64_t state = 1;

static uint64_t next(void)
{
	state = state * 6364136223846793005ULL + 1442695040888963407ULL;
	return state ^ (state >> 29);
}

void t_randseed(uint64_t s)
{
	state = s;
}

/* Uniform in [0, n) for n > 0: reject the incomplete final block of 2^64 values */
uint64_t t_randn(uint64_t n)
{
	uint64_t limit = UINT64_MAX - UINT64_MAX % n;
	uint64_t r;

	do
		r = next();
	while (r >= limit);

	return r % n;
}

/* Uniform in [a, b] for a <= b */
uint64_t t_randint(uint64_t a, uint64_t b)
{
	uint64_t n = b - a + 1;

	return n == 0 ? next() : a + t_randn(n);
}

void t_shuffle(uint64_t *p, size_t n)
{
	for (size_t i = n; i > 1; --i) {
		size_t j = t_randn(i);
		uint64_t t = p[i - 1];

		p[i - 1] = p[j];
		p[j] = t;
	}
}

void t_randrange(uint64_t *p, size_t n)
{
	for (size_t i = 0; i < n; ++i)
		p[i] = i;
	t_shuffle(p, n);
}
