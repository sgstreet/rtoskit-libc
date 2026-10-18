/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * stdckdint.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <limits.h>
#include <stdckdint.h>
#include <stdint.h>
#include <stdio.h>

/*
 * <stdckdint.h> (P1.03): results as if computed in infinite precision,
 * stored wrapped, with true on overflow; mixed signedness and widths.
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

int main(void)
{
	int i;
	unsigned int u;
	long long ll;
	unsigned char uc;
	signed char sc;
	unsigned long long ull;

	CHECK(!ckd_add(&i, 1, 2) && i == 3);
	CHECK(ckd_add(&i, INT_MAX, 1) && i == INT_MIN);
	CHECK(ckd_sub(&i, INT_MIN, 1) && i == INT_MAX);
	CHECK(!ckd_sub(&i, -1, INT_MAX) && i == INT_MIN);
	CHECK(ckd_mul(&i, INT_MAX, 2) && i == -2);
	CHECK(!ckd_mul(&i, -46341, 46340) && i == -2147441940);

	/* Unsigned results of negative mathematical values wrap */
	CHECK(ckd_sub(&u, 0, 1) && u == UINT_MAX);
	CHECK(ckd_add(&u, UINT_MAX, 1U) && u == 0);
	CHECK(!ckd_add(&u, -1, 2) && u == 1);

	/* Mixed widths: the operation is on mathematical values (C23 7.20.1 example) */
	CHECK(!ckd_sub(&ll, INT_MIN, INT_MAX) && ll == (long long)INT_MIN - INT_MAX);
	CHECK(!ckd_mul(&ll, INT_MAX, INT_MAX) && ll == (long long)INT_MAX * INT_MAX);
	CHECK(ckd_mul(&ull, ULLONG_MAX, 2ULL) && ull == ULLONG_MAX - 1);
	CHECK(!ckd_add(&ull, LLONG_MAX, LLONG_MAX) && ull == 2ULL * LLONG_MAX);
	CHECK(ckd_sub(&ull, 0, LLONG_MIN) == false && ull == 1ULL << 63);

	/* Narrow results */
	CHECK(ckd_add(&uc, 200, 100) && uc == 44);
	CHECK(!ckd_add(&uc, 200, 55) && uc == 255);
	CHECK(ckd_sub(&sc, -100, 100) && sc == 56);
	CHECK(!ckd_mul(&sc, -16, 8) && sc == -128);

	static_assert(__STDC_VERSION_STDCKDINT_H__ == 202311L);

	if (failures == 0)
		puts("pass: stdckdint");

	return failures == 0 ? 0 : 1;
}
