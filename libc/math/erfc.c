/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * erfc.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.8.2, F.10.5.2: 1 - erf below 1/2, the table to 4, the continued
 * fraction to 27.3 (scaled once, so subnormal results round once), 2 - erfc
 * for negative x; underflow beyond 27.3.
 */
double erfc(double x)
{
	double a = fabs(x);
	struct __libc_dd value;
	int exponent;
	int mode;

	if (isnan(x))
		return x + x;
	if (isinf(x))
		return x > 0 ? 0.0 : 2.0;
	if (x >= 27.3)
		return __libc_math_underflow(1.0);
	if (x <= -6.0) {
		volatile double tiny = 0x1p-80;

		return 2.0 - tiny;
	}
	if (a < 0x1p-56)
		return 1.0 - x;

	mode = __libc_math_nearest();
	if (a < 0.5) {
		value = __libc_dd_add((struct __libc_dd){ .hi = 1.0, .lo = 0.0 }, __libc_dd_negate(__libc_erf_series(x)));
	} else if (a < 4.0) {
		value = __libc_erfc_table(a);
		if (x < 0)
			value = __libc_dd_add((struct __libc_dd){ .hi = 2.0, .lo = 0.0 }, __libc_dd_negate(value));
	} else {
		value = __libc_erfc_tail(a, &exponent);
		if (x > 0) {
			__libc_math_restore(mode);
			return __libc_exp_scale(value, exponent);
		}
		value = __libc_dd_add((struct __libc_dd){ .hi = 2.0, .lo = 0.0 }, (struct __libc_dd){ .hi = -scalbn(value.hi, exponent), .lo = -scalbn(value.lo, exponent) });
	}
	__libc_math_restore(mode);

	return value.hi + value.lo;
}
