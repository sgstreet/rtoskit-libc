/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * erfcf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.8.2: below 1, 1 - erf(x) with the erf core (the subtraction
 * loses at most 3 bits there); from 1, Laplace's continued fraction
 * erfc(x) = e^-x^2 / (sqrt(pi) (x + (1/2)/(x + 1/(x + (3/2)/(x + ...)))))
 * as float-float with 150/x^2 + 10 levels (enough for 2^-46 at every x),
 * scaled once so that subnormal results round once; 2 below -3.9 and 0
 * beyond 10.06 (PLAN D63).
 */
float erfcf(float x)
{
	struct __libc_ff value;
	struct __libc_ff square;
	struct __libc_ff fraction;
	int levels;
	int exponent;

	if (isnan(x))
		return x + x;
	if (x < -3.9f)
		return 2.0f;
	if (x > 10.06f)
		return 0x1p-100f * 0x1p-100f;
	/* erfc(x) = 1 - 2x/sqrt(pi) rounds to 1 - x */
	if (fabsf(x) < 0x1p-26f)
		return 1.0f - x;

	if (x < 1.0f) {
		value = __libc_erff_core(fabsf(x));
		if (x > 0)
			value = (struct __libc_ff){ .hi = -value.hi, .lo = -value.lo };
		value = __libc_ff_add((struct __libc_ff){ .hi = 1.0f, .lo = 0.0f }, value);
		return value.hi + value.lo;
	}

	levels = (int)(150.0f / (x * x)) + 10;
	fraction = (struct __libc_ff){ .hi = x, .lo = 0.0f };
	for (int k = levels; k > 0; --k)
		fraction = __libc_ff_add((struct __libc_ff){ .hi = x, .lo = 0.0f }, __libc_ff_divide((struct __libc_ff){ .hi = 0.5f * (float)k, .lo = 0.0f }, fraction));

	square = __libc_ff_product(x, x);
	value = __libc_exp2f_core(__libc_ff_multiply((struct __libc_ff){ .hi = -square.hi, .lo = -square.lo }, __LIBC_INV_LN2F), &exponent);
	value = __libc_ff_divide(__libc_ff_multiply(value, __LIBC_INV_SQRTPIF), fraction);

	return __libc_exp2f_scale(value, exponent);
}
