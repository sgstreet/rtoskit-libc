/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * asinpif.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.4.9: asin x times 1/pi as float-float; asinpi(+-1) is +-1/2 exactly (PLAN D61) */
float asinpif(float x)
{
	struct __libc_ff value;
	int shift;

	if (isnan(x) || x == 0)
		return x + x;
	if (isgreater(fabsf(x), 1.0f))
		return __libc_math_invalidf();
	if (fabsf(x) == 1.0f)
		return copysignf(0.5f, x);
	/* Below 2^-19 the x^3 term is under 2^-39 of the result: x/pi rounded once, subnormal results included */
	if (fabsf(x) < 0x1p-19f) {
		shift = ilogbf(x);
		return __libc_exp2f_scale(__libc_ff_scale(scalbnf(x, -shift), __LIBC_INV_PIF), shift);
	}

	value = __libc_atan2f_core((struct __libc_ff){ .hi = x, .lo = 0.0f }, __libc_ff_sqrt(__libc_ff_multiply(__libc_ff_sum(1.0f, -x), __libc_ff_sum(1.0f, x))));
	value = __libc_ff_multiply(value, __LIBC_INV_PIF);

	return value.hi + value.lo;
}
