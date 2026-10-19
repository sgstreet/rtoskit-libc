/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * exp2.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.6.4: integers exactly; otherwise exp(x ln2) with the product as a double-double */
double exp2(double x)
{
	struct __libc_dd z;
	struct __libc_dd value;
	int exponent;

	if (isnan(x))
		return x + x;
	if (isinf(x))
		return x > 0 ? x : 0.0;
	if (x >= 1024)
		return __libc_math_overflow(1.0);
	if (x <= -1075)
		return __libc_math_underflow(1.0);
	if (x == trunc(x))
		return scalbn(1.0, (int)x);
	if (fabs(x) < 0x1p-54)
		return 1.0 + x;

	z = __libc_dd_product(x, __LIBC_LN2.hi);
	z = __libc_dd_fast_sum(z.hi, z.lo + x * __LIBC_LN2.lo);
	value = __libc_exp_core(z.hi, z.lo, &exponent);
	return __libc_exp_scale(value, exponent);
}
