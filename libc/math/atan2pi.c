/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * atan2pi.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.4.12; a tiny quotient is scaled by 2^200 and rounded once: the angle divided by pi as a double-double (docs/research/libm-trig.md) */
double atan2pi(double y, double x)
{
	struct __libc_dd value;
	int scale;
	int mode;

	if (isnan(x) || isnan(y))
		return x + y;
	if (y == 0)
		return signbit(x) ? copysign(1.0, y) : y;
	if (x == 0)
		return copysign(0.5, y);
	if (isinf(y))
		return copysign(isinf(x) ? (x > 0 ? 0.25 : 0.75) : 0.5, y);
	if (isinf(x))
		return x > 0 ? copysign(0.0, y) : copysign(1.0, y);
	if (fabs(x) == fabs(y))
		return copysign(x > 0 ? 0.25 : 0.75, y);

	if (ilogb(y) - ilogb(x) > 60) {
		volatile double tiny = 0x1p-80;

		return copysign(x > 0 ? 0.5 - tiny : 0.5 + tiny, y);
	}
	if (x < 0 && ilogb(y) - ilogb(x) < -60) {
		volatile double tiny = 0x1p-80;

		return copysign(1.0 - tiny, y);
	}

	scale = -(ilogb(fabs(x) > fabs(y) ? x : y));
	if (x > 0 && ilogb(y) - ilogb(x) < -60) {
		mode = __libc_math_nearest();
		value = __libc_dd_divide((struct __libc_dd){ .hi = scalbn(y, scale + 200), .lo = 0.0 }, (struct __libc_dd){ .hi = scalbn(x, scale), .lo = 0.0 });
		value = __libc_dd_multiply(value, __LIBC_INV_PI);
		__libc_math_restore(mode);
		return __libc_exp_scale(value, -200);
	}

	mode = __libc_math_nearest();
	value = __libc_atan2_core((struct __libc_dd){ .hi = scalbn(y, scale), .lo = 0.0 }, (struct __libc_dd){ .hi = scalbn(x, scale), .lo = 0.0 });
	value = __libc_dd_multiply(value, __LIBC_INV_PI);
	__libc_math_restore(mode);

	return value.hi + value.lo;
}
