/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * powr.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.7.7, F.10.4.7: exp(y log x), defined for x >= 0 only, without
 * pow's integer cases: powr(0, 0), powr(+infinity, 0) and powr(1, +-infinity)
 * are domain errors, NaNs always propagate, and powr(0, -infinity) is
 * +infinity without divide-by-zero (IEEE 754 9.2.1).
 */
double powr(double x, double y)
{
	struct __libc_dd log;
	struct __libc_dd z;
	struct __libc_dd value;
	int exponent;

	if (isnan(x) || isnan(y))
		return x + y;
	if (x < 0)
		return __libc_math_invalid();
	if (x == 0) {
		if (y == 0)
			return __libc_math_invalid();
		if (y < 0)
			return isinf(y) ? INFINITY : __libc_math_divbyzero(1.0);
		return 0.0;
	}
	if (isinf(x)) {
		if (y == 0)
			return __libc_math_invalid();
		return y < 0 ? 0.0 : INFINITY;
	}
	if (x == 1.0)
		return isinf(y) ? __libc_math_invalid() : 1.0;
	if (isinf(y))
		return (x < 1.0) == (y < 0) ? INFINITY : 0.0;
	if (y == 0)
		return 1.0;

	log = __libc_log_core(x);
	if (y * log.hi > 710)
		return __libc_math_overflow(1.0);
	if (y * log.hi < -746)
		return __libc_math_underflow(1.0);

	z = __libc_dd_product(y, log.hi);
	z = __libc_dd_sum(z.hi, z.lo + y * log.lo);
	value = __libc_exp_core(z.hi, z.lo, &exponent);
	return __libc_exp_scale(value, exponent);
}
