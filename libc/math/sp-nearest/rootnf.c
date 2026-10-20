/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * rootnf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.7.8: x^(1/n) = 2^(log2 |x| / n), the quotient as float-float (PLAN D59) */
float rootnf(float x, long long n)
{
	bool odd = (n & 1) != 0;
	bool negative = signbit(x) && odd;

	if (n == 0)
		return __libc_math_invalidf();
	if (isnan(x))
		return x + x;
	if (signbit(x) && !odd && x != 0)
		return __libc_math_invalidf();
	if (x == 0) {
		if (n < 0)
			return (negative ? -1.0f : 1.0f) / fabsf(x);
		return odd ? x : 0.0f;
	}
	if (isinf(x)) {
		float result = n < 0 ? 0.0f : INFINITY;

		return negative ? -result : result;
	}
	if (n == 1)
		return x;

	return __libc_exp2f_signed(__libc_ff_divide(__libc_log2f_core(fabsf(x), 0.0f), __libc_ff_from_i64(n)), negative);
}
