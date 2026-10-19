/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cospi.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.4.11: cos(pi x) with an exact reduction x = n/2 + r, |r| <= 1/4;
 * cospi(n + 1/2) is +0 for integers n (IEEE 754 9.2.1).
 */
double cospi(double x)
{
	double n;
	double r;
	struct __libc_dd angle;
	struct __libc_dd value;
	int quadrant;
	int mode;

	if (isnan(x))
		return x + x;
	if (isinf(x))
		return __libc_math_invalid();
	if (fabs(x) >= 0x1p53)
		return 1.0;
	if (fabs(x) >= 0x1p52)
		return fmod(x, 2.0) == 0 ? 1.0 : -1.0;

	n = roundeven(2.0 * x);
	r = x - 0.5 * n;
	quadrant = (int)fmod(n, 4.0) & 3;
	if (r == 0)
		return (quadrant & 1) != 0 ? 0.0 : (quadrant == 0 ? 1.0 : -1.0);

	mode = __libc_math_nearest();
	angle = __libc_dd_product(r, __LIBC_PI.hi);
	angle = __libc_dd_fast_sum(angle.hi, angle.lo + r * __LIBC_PI.lo);
	switch (quadrant) {
		case 0:
			value = __libc_cos_core(angle);
			break;
		case 1:
			value = __libc_dd_negate(__libc_sin_core(angle));
			break;
		case 2:
			value = __libc_dd_negate(__libc_cos_core(angle));
			break;
		default:
			value = __libc_sin_core(angle);
			break;
	}
	__libc_math_restore(mode);

	return value.hi + value.lo;
}
