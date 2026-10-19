/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * bitint.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <limits.h>
#include <stdbit.h>
#include <stdio.h>

/*
 * Bit-precise integer support in the headers: BITINT_MAXWIDTH (C23 5.2.5.3.2)
 * and the <stdbit.h> type-generic macros on _BitInt arguments (C23 7.18.1p3).
 * Needs a compiler that implements _BitInt for the target (GCC 16 on Arm, Q12).
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

static_assert(BITINT_MAXWIDTH >= ULLONG_WIDTH);

int main(void)
{
	/* Bit-precise types whose width matches a standard type */
	CHECK(stdc_count_ones((unsigned _BitInt(32))0xf0f0f0f0) == 16);
	CHECK(stdc_bit_ceil((unsigned _BitInt(16))0x8001) == 0);

	/* And widths no standard type has */
	CHECK(stdc_leading_zeros((unsigned _BitInt(7))1) == 6);
	CHECK(stdc_bit_width((unsigned _BitInt(65))1 << 64) == 65);

	if (failures == 0)
		puts("pass: bitint");

	return failures == 0 ? 0 : 1;
}
