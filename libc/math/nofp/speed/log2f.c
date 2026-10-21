/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * log2f.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"
#include "libm-fixed.h"

/*
 * C23 7.12.6.16: for a positive finite x through the fixed-point core; the
 * float-float path (PLAN D58) takes the other arguments and the results the
 * fixed-point one cannot round with certainty (PLAN D86). log2f(2^n) is n
 * exactly: the low part is 0.
 */
float log2f(float x)
{
	uint32_t bits = __libc_float_bits(x);
	struct __libc_ff value;

	if (bits - 1 < 0x7f7fffff) {
		bool negative;
		int exponent;
		uint64_t v = __libc_log2f_fixed(bits, &exponent, &negative);
		uint32_t result;

		if (v == 0)
			return 0.0f;
		if (__libc_fixed_round((uint32_t)(v >> 31), exponent, __LIBC_FIXED_LOG2_ERROR, &result))
			return __libc_float_from(result | (negative ? 0x80000000u : 0));
	}

	if (isnan(x))
		return x + x;
	if (x < 0)
		return __libc_math_invalidf();
	/* log(+-0) is -infinity with divide-by-zero */
	if (x == 0)
		return -1.0f / fabsf(x);
	if (isinf(x))
		return x;

	value = __libc_log2f_core(x, 0.0f);

	return value.hi + value.lo;
}
