/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * exp10m1f.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.6.3: 10^x - 1. Near 0 the series on r = x ln 10 keeps the
 * relative accuracy; elsewhere the exp2 core's result minus 1 is a
 * float-float sum, rounded once (PLAN D57).
 */
float exp10m1f(float x)
{
	int exponent;
	struct __libc_ff value;

	if (isnan(x))
		return x + x;
	if (x > 39.0f)
		return x * 0x1p127f;
	if (x < -8.0f)
		return -1.0f + 0x1p-100f;
	/*
	 * Far below 1 the result is x c rounded once; x is scaled into [1, 2)
	 * first so that the exact product does not underflow, and the scaling
	 * back rounds once, subnormal results included.
	 */
	if (fabsf(x) < 0x1p-90f) {
		int shift;

		if (x == 0)
			return x;
		shift = ilogbf(x);
		return __libc_exp2f_scale(__libc_ff_scale(scalbnf(x, -shift), __LIBC_LN10F), shift);
	}
	if (fabsf(x) < 0x1p-8f) {
		value = __libc_ff_expm1_series(__libc_ff_scale(x, __LIBC_LN10F));
		return value.hi + value.lo;
	}

	value = __libc_exp2f_core(__libc_ff_scale(x, __LIBC_LOG2_10F), &exponent);

	return __libc_exp2m1f_scale(value, exponent);
}
