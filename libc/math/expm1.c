/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * expm1.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.6.6: exp(x) - 1, accurate near 0 */
double expm1(double x)
{
	struct __libc_dd z;

	if (isnan(x))
		return x + x;
	if (isinf(x))
		return x > 0 ? x : -1.0;
	if (x > 709.79)
		return __libc_math_overflow(1.0);
	if (x < -40) {
		volatile double tiny = 0x1p-100;

		return tiny - 1.0;
	}
	if (fabs(x) < 0x1p-60)
		return __libc_math_tiny(x, 1.0);

	z = (struct __libc_dd){ .hi = x, .lo = 0.0 };
	return __libc_expm1_core(z.hi, z.lo);
}
