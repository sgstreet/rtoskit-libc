/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * atan.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.4.3, F.10.1.3: atan(+-infinity) is +-pi/2, as is every |x| beyond 2^60 once rounded */
double atan(double x)
{
	struct __libc_dd value;
	int mode;

	if (isnan(x))
		return x + x;
	if (fabs(x) < 0x1p-27)
		return __libc_math_tiny(x, -x);
	if (isinf(x) || fabs(x) > 0x1p60)
		return copysign(__LIBC_PI_2.hi, x) + copysign(__LIBC_PI_2.lo, x);

	mode = __libc_math_nearest();
	value = __libc_atan2_core((struct __libc_dd){ .hi = x, .lo = 0.0 }, (struct __libc_dd){ .hi = 1.0, .lo = 0.0 });
	__libc_math_restore(mode);

	return value.hi + value.lo;
}
