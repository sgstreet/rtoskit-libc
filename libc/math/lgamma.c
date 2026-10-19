/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * lgamma.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.8.3, F.10.5.3: lgamma(1) = lgamma(2) = +0, a pole (zero or a
 * negative integer) is +infinity with divide-by-zero; x (log x - 1) beyond
 * 2^60, rounded once (it overflows past 2.5e305).
 */
double lgamma(double x)
{
	struct __libc_dd value;
	int sign;
	int mode;

	if (isnan(x))
		return x + x;
	if (isinf(x))
		return fabs(x);
	if (x <= 0 && x == floor(x))
		return __libc_math_divbyzero(1.0);
	if (x == 1.0 || x == 2.0)
		return 0.0;
	if (x > 0x1p60) {
		/* x (log x - 1) with x = m 2^e: m (log x - 1) as a double-double, then one scaled rounding (Stirling's other terms are below 2^-60 relative) */
		int exponent = ilogb(x);

		mode = __libc_math_nearest();
		value = __libc_dd_multiply(__libc_dd_add(__libc_log_core(x), (struct __libc_dd){ .hi = -1.0, .lo = 0.0 }), (struct __libc_dd){ .hi = scalbn(x, -exponent), .lo = 0.0 });
		__libc_math_restore(mode);
		return __libc_exp_scale(value, exponent);
	}
	if (fabs(x) < 0x1p-60)
		return -log(fabs(x));

	mode = __libc_math_nearest();
	value = __libc_lgamma_core(x, &sign);
	__libc_math_restore(mode);

	return value.hi + value.lo;
}
