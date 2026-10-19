/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * atanh.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.5.3, F.10.2.3: log1p(2|x|/(1 - |x|))/2; atanh(+-1) is +-infinity (divide-by-zero), |x| > 1 a domain error */
double atanh(double x)
{
	struct __libc_dd value;
	int mode;
	double a = fabs(x);

	if (isnan(x) || x == 0)
		return x + x;
	if (a > 1.0)
		return __libc_math_invalid();
	if (a == 1.0)
		return __libc_math_divbyzero(copysign(1.0, x));
	if (a < 0x1p-27)
		return __libc_math_tiny(x, x);

	mode = __libc_math_nearest();
	value = __libc_dd_divide((struct __libc_dd){ .hi = 2.0 * a, .lo = 0.0 }, __libc_dd_sum(1.0, -a));
	value = __libc_log1p_dd(value);
	value = (struct __libc_dd){ .hi = 0.5 * value.hi, .lo = 0.5 * value.lo };
	if (signbit(x))
		value = __libc_dd_negate(value);
	__libc_math_restore(mode);

	return value.hi + value.lo;
}
