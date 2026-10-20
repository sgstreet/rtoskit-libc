/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * sqrtf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.7.10, correctly rounded in the current direction: x = m 2^s with
 * a 24-bit m; the integer root of m 2^t (t = 29 or 30, so that s - t is
 * even) has 26 or 27 bits, its last bit or'ed with "not exact" (sticky), and
 * one conversion of that integer to float rounds it as the operation must
 * (the conversion follows the rounding direction). No double (PLAN D59).
 */
float sqrtf(float x)
{
	uint32_t bits = __libc_float_bits(x);
	int exponent = __libc_float_exponent(bits);
	uint32_t significand = bits & __LIBC_FLOAT_FRACTION;
	int shift;
	bool inexact;
	uint64_t root;

	if (isnan(x) || x == 0 || isinf(x))
		return x < 0 ? __libc_math_invalidf() : x + x;
	if (x < 0)
		return __libc_math_invalidf();

	if (exponent == 0) {
		for (exponent = 1; significand < (1U << 23); --exponent)
			significand <<= 1;
	} else {
		significand |= 1U << 23;
	}

	/* x = significand 2^(exponent - 150) */
	exponent -= __LIBC_FLOAT_BIAS + 23;
	shift = (exponent & 1) != 0 ? 29 : 30;
	root = __libc_isqrt(0, (uint64_t)significand << shift, 27, &inexact);

	return scalbnf((float)(uint32_t)(root | (inexact ? 1 : 0)), (exponent - shift) / 2);
}
