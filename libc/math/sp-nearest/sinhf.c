/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * sinhf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.5.5: below 1/4 the odd series x + x^3/6 + x^5/120 + x^7/5040 +
 * x^9/362880 (cube and 1/6 as float-float); above, (e^|x| - e^-|x|)/2 from
 * the exp2 core with one rounding, the sign of x restored (PLAN D62).
 */
float sinhf(float x)
{
	struct __libc_ff value;
	struct __libc_ff inverse;
	struct __libc_ff square;
	int exponent;

	if (isnan(x) || isinf(x))
		return x + x;
	/* The x^3/6 term is below half an ulp of x */
	if (fabsf(x) < 0x1p-12f)
		return x;
	if (fabsf(x) > 90.0f)
		return x * 0x1p127f;

	if (fabsf(x) < 0.25f) {
		square = __libc_ff_product(x, x);
		value = __libc_ff_multiply(__libc_ff_multiply((struct __libc_ff){ .hi = x, .lo = 0.0f }, square), __libc_ff_fast_sum(0x1.555556p-3f, -0x1.555556p-28f + square.hi * (0x1.111112p-7f + square.hi * (0x1.a01a02p-13f + square.hi * 0x1.71de3ap-19f))));
		value = __libc_ff_add((struct __libc_ff){ .hi = x, .lo = 0.0f }, value);
		return value.hi + value.lo;
	}

	value = __libc_expf_pair(x, &exponent, &inverse);
	value = __libc_ff_add(value, (struct __libc_ff){ .hi = -inverse.hi, .lo = -inverse.lo });

	return copysignf(__libc_exp2f_scale(value, exponent - 1), x);
}
