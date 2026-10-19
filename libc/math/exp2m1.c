/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * exp2m1.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.6.5: 2^x - 1, through expm1 of x ln2 as a double-double */
double exp2m1(double x)
{
	struct __libc_dd z;

	if (isnan(x))
		return x + x;
	if (isinf(x))
		return x > 0 ? x : -1.0;
	if (x > 1024.0)
		return __libc_math_overflow(1.0);
	if (x < -60) {
		volatile double tiny = 0x1p-100;

		return tiny - 1.0;
	}
	if (fabs(x) < 0x1p-60)
		return x * __LIBC_LN2.hi;

	z = __libc_dd_product(x, __LIBC_LN2.hi);
	z = __libc_dd_fast_sum(z.hi, z.lo + x * __LIBC_LN2.lo);
	return __libc_expm1_core(z.hi, z.lo);
}
