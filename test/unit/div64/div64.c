/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * div64.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <stdio.h>

#include "internal/div64.h"

/*
 * The library's 64-bit division by small divisors (P2.04, PLAN D54) against
 * the compiler's own division: the divisors the library uses, the extremes
 * of the range, and random ones, with random and edge dividends.
 */

static int failures;

static void fail(const char *what, uint64_t n, uint32_t divisor)
{
	char message[128];

	snprintf(message, sizeof(message), "fail: %s n=%llu divisor=%lu\n", what, (unsigned long long)n, (unsigned long)divisor);
	fputs(message, stdout);
	++failures;
}

/* xorshift64: deterministic test values */
static uint64_t next(uint64_t *state)
{
	*state ^= *state << 13;
	*state ^= *state >> 7;
	*state ^= *state << 17;
	return *state;
}

static void check(uint64_t n, uint32_t divisor)
{
	uint32_t remainder;
	uint64_t quotient = __libc_udivmod64(n, divisor, &remainder);
	int64_t s = (int64_t)n;
	int64_t floor_quotient = s / (int64_t)divisor;
	int64_t floor_remainder = s % (int64_t)divisor;
	uint32_t got_remainder;
	int64_t got_quotient = __libc_floor_divmod64(s, divisor, &got_remainder);

	if (quotient != n / divisor || remainder != n % divisor)
		fail("unsigned", n, divisor);

	/* The reference rounds toward zero; floor division moves a negative remainder up by one divisor */
	if (floor_remainder < 0) {
		floor_remainder += divisor;
		--floor_quotient;
	}
	if (got_quotient != floor_quotient || got_remainder != (uint64_t)floor_remainder)
		fail("floor", n, divisor);
}

int main(void)
{
	static const uint32_t divisors[] = { 1, 2, 3, 7, 10, 16, 100, 400, 3600, 86400, 146097, 1000000007, 0x7fffffffu, 0x80000000u };
	static const uint64_t edges[] = { 0, 1, 9, 10, 11, UINT32_MAX, (uint64_t)UINT32_MAX + 1, INT64_MAX, (uint64_t)INT64_MAX + 1, UINT64_MAX, UINT64_MAX - 1 };
	uint64_t state = 0x9e3779b97f4a7c15u;

	for (size_t d = 0; d < sizeof(divisors) / sizeof(divisors[0]); ++d) {
		for (size_t e = 0; e < sizeof(edges) / sizeof(edges[0]); ++e)
			check(edges[e], divisors[d]);
		for (int i = 0; i < 2000; ++i)
			check(next(&state), divisors[d]);
	}

	for (int i = 0; i < 20000; ++i) {
		uint32_t divisor = (uint32_t)(next(&state) % 0x80000000u) + 1;

		check(next(&state), divisor);
		check(next(&state) >> (next(&state) % 64), divisor);
	}

	if (failures == 0)
		puts("pass: div64");

	return failures == 0 ? 0 : 1;
}
