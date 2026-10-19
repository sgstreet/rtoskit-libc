/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * exp.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.6.1, F.10.3.1 */
double exp(double x)
{
	struct __libc_dd value;
	int exponent;

	if (isnan(x))
		return x + x;
	if (isinf(x))
		return x > 0 ? x : 0.0;
	if (x > 709.79)
		return __libc_math_overflow(1.0);
	if (x < -745.14)
		return __libc_math_underflow(1.0);
	if (fabs(x) < 0x1p-54)
		return 1.0 + x;

	value = __libc_exp_core(x, 0.0, &exponent);
	return __libc_exp_scale(value, exponent);
}
