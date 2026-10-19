/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * rsqrt.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.7.9: 1/sqrt(x): rsqrt(+-0) is +-infinity with divide-by-zero, a negative x a domain error */
double rsqrt(double x)
{
	struct __libc_dd log;
	struct __libc_dd value;
	int exponent;

	if (isnan(x))
		return x + x;
	if (x == 0)
		return __libc_math_divbyzero(signbit(x) ? -1.0 : 1.0);
	if (x < 0)
		return __libc_math_invalid();
	if (isinf(x))
		return 0.0;

	log = __libc_log_core(x);
	value = __libc_exp_core(-0.5 * log.hi, -0.5 * log.lo, &exponent);
	return __libc_exp_scale(value, exponent);
}
