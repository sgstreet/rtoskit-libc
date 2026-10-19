/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * sinh.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.5.5, F.10.2.5: the series below 1, (e^|x| - e^-|x|)/2 up to 22,
 * e^|x|/2 beyond (scaled once, so the overflow threshold is exact).
 */
double sinh(double x)
{
	struct __libc_dd value;
	struct __libc_dd e;
	int exponent;
	int mode;
	double a = fabs(x);

	if (isnan(x) || isinf(x) || x == 0)
		return x + x;
	if (a < 0x1p-27)
		return __libc_math_tiny(x, x);
	if (a > 711.0)
		return __libc_math_overflow(copysign(1.0, x));

	mode = __libc_math_nearest();
	if (a < 1.0) {
		value = __libc_sinh_core(a);
	} else if (a < 22.0) {
		e = __libc_exp_dd(a);
		value = __libc_dd_add(e, __libc_dd_negate(__libc_dd_divide((struct __libc_dd){ .hi = 1.0, .lo = 0.0 }, e)));
		value = (struct __libc_dd){ .hi = 0.5 * value.hi, .lo = 0.5 * value.lo };
	} else {
		value = __libc_exp_core(a, 0.0, &exponent);
		if (signbit(x))
			value = __libc_dd_negate(value);
		__libc_math_restore(mode);
		return __libc_exp_scale(value, exponent - 1);
	}
	if (signbit(x))
		value = __libc_dd_negate(value);
	__libc_math_restore(mode);

	return value.hi + value.lo;
}
