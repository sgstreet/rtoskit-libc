/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * pownf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.7.6: x^n = 2^(n log2 |x|), n as float-float from its three 21-bit chunks (PLAN D59) */
float pownf(float x, long long n)
{
	bool negative = signbit(x) && (n & 1) != 0;

	if (n == 0)
		return 1.0f;
	if (isnan(x))
		return x + x;
	if (x == 0) {
		if (n < 0)
			return (negative ? -1.0f : 1.0f) / fabsf(x);
		return negative ? -0.0f : 0.0f;
	}
	if (isinf(x)) {
		float result = n < 0 ? 0.0f : INFINITY;

		return negative ? -result : result;
	}
	if (fabsf(x) == 1.0f)
		return negative ? -1.0f : 1.0f;
	if (n == 1)
		return x;

	return __libc_exp2f_signed(__libc_ff_multiply(__libc_log2f_core(fabsf(x), 0.0f), __libc_ff_from_i64(n)), negative);
}
