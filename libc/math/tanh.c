/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tanh.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.5.6, F.10.2.6: sinh/cosh below 1, 1 - 2/(e^2|x| + 1) up to 22, +-1 (inexact) beyond */
double tanh(double x)
{
	struct __libc_dd value;
	struct __libc_dd e;
	int mode;
	double a = fabs(x);

	if (isnan(x) || x == 0)
		return x + x;
	if (isinf(x))
		return copysign(1.0, x);
	if (a < 0x1p-27)
		return __libc_math_tiny(x, -x);
	if (a >= 22.0) {
		volatile double tiny = 0x1p-80;

		return x > 0 ? 1.0 - tiny : tiny - 1.0;
	}

	mode = __libc_math_nearest();
	if (a < 1.0) {
		value = __libc_dd_divide(__libc_sinh_core(a), __libc_cosh_core(a));
	} else {
		e = __libc_exp_dd(2.0 * a);
		value = __libc_dd_divide((struct __libc_dd){ .hi = 2.0, .lo = 0.0 }, __libc_dd_add(e, (struct __libc_dd){ .hi = 1.0, .lo = 0.0 }));
		value = __libc_dd_fast_sum(1.0, -value.hi - value.lo);
	}
	if (signbit(x))
		value = __libc_dd_negate(value);
	__libc_math_restore(mode);

	return value.hi + value.lo;
}
