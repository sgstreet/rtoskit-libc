/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * erf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.8.1, F.10.5.1: the series below 1/2, 1 - erfc above, +-1 (inexact) from 6 */
double erf(double x)
{
	const struct __libc_dd two_sqrtpi = { .hi = 0x1.20dd750429b6dp+0, .lo = 0x1.1ae3a914fed80p-56 };
	double a = fabs(x);
	struct __libc_dd value;
	struct __libc_dd tail;
	int exponent;
	int mode;

	if (isnan(x) || x == 0)
		return x + x;
	if (isinf(x))
		return copysign(1.0, x);
	if (a < 0x1p-28)
		return __libc_dd_scale_product(x, two_sqrtpi);
	if (a >= 6.0) {
		volatile double tiny = 0x1p-80;

		return x > 0 ? 1.0 - tiny : tiny - 1.0;
	}

	mode = __libc_math_nearest();
	if (a < 0.5) {
		value = __libc_erf_series(a);
	} else if (a < 4.0) {
		value = __libc_dd_add((struct __libc_dd){ .hi = 1.0, .lo = 0.0 }, __libc_dd_negate(__libc_erfc_table(a)));
	} else {
		tail = __libc_erfc_tail(a, &exponent);
		value = __libc_dd_add((struct __libc_dd){ .hi = 1.0, .lo = 0.0 }, (struct __libc_dd){ .hi = -scalbn(tail.hi, exponent), .lo = -scalbn(tail.lo, exponent) });
	}
	if (signbit(x))
		value = __libc_dd_negate(value);
	__libc_math_restore(mode);

	return value.hi + value.lo;
}
