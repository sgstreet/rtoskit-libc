/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * powf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* 1 for an odd integer y, 0 for an even one (every |y| from 2^24 up), -1 when y is not an integer */
static int integer_kind(float y)
{
	if (truncf(y) != y)
		return -1;
	if (fabsf(y) >= 0x1p24f)
		return 0;

	return ((int32_t)y & 1) != 0 ? 1 : 0;
}

/*
 * C23 7.12.7.5, F.10.4.5: the special cases, then
 * 2^(y log2 |x|) with the logarithm and the product as float-float; the
 * sign comes from an odd integer y (PLAN D59).
 */
float powf(float x, float y)
{
	int kind;
	bool negative;
	struct __libc_ff log;
	struct __libc_ff z;

	if (y == 0 || x == 1.0f)
		return 1.0f;
	if (isnan(x) || isnan(y))
		return x + y;
	if (isinf(y)) {
		if (fabsf(x) == 1.0f)
			return 1.0f;
		return (fabsf(x) < 1.0f) == (y < 0) ? INFINITY : 0.0f;
	}

	kind = integer_kind(y);
	negative = signbit(x) && kind == 1;
	if (x == 0) {
		/* +-infinity with divide-by-zero for a negative y */
		if (y < 0)
			return (negative ? -1.0f : 1.0f) / fabsf(x);
		return negative ? -0.0f : 0.0f;
	}
	if (isinf(x)) {
		float result = y < 0 ? 0.0f : INFINITY;

		return negative ? -result : result;
	}
	if (x < 0 && kind < 0)
		return __libc_math_invalidf();
	if (y == 1.0f)
		return x;

	log = __libc_log2f_core(fabsf(x), 0.0f);
	/* |log2 |x|| is at least 2^-23 here, so a |y| above 2^31 is far out of range either way */
	if (fabsf(y) > 0x1p31f)
		z = (struct __libc_ff){ .hi = y * log.hi, .lo = 0.0f };
	else
		z = __libc_ff_multiply(log, (struct __libc_ff){ .hi = y, .lo = 0.0f });

	return __libc_exp2f_signed(z, negative);
}
