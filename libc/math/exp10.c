/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * exp10.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.6.2: 10^0..10^22 exactly; otherwise exp(x ln10) with the product as a double-double */
double exp10(double x)
{
	struct __libc_dd z;
	struct __libc_dd value;
	int exponent;

	if (isnan(x))
		return x + x;
	if (isinf(x))
		return x > 0 ? x : 0.0;
	if (x > 308.26)
		return __libc_math_overflow(1.0);
	if (x < -323.61)
		return __libc_math_underflow(1.0);
	if (x >= 0 && x <= 22 && x == trunc(x)) {
		double power = 1.0;

		for (int i = 0; i < (int)x; ++i)
			power *= 10.0;
		return power;
	}
	if (fabs(x) < 0x1p-56)
		return 1.0 + x;

	z = __libc_dd_product(x, __LIBC_LN10.hi);
	z = __libc_dd_fast_sum(z.hi, z.lo + x * __LIBC_LN10.lo);
	value = __libc_exp_core(z.hi, z.lo, &exponent);
	return __libc_exp_scale(value, exponent);
}
