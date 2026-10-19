/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * pown.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.7.6: x^n for a 64-bit integer n, with pow's special cases for an integer exponent */
double pown(double x, long long n)
{
	bool negative = signbit(x) && (n & 1) != 0;

	if (n == 0)
		return 1.0;
	if (isnan(x))
		return x + x;
	if (x == 0) {
		if (n < 0)
			return __libc_math_divbyzero(negative ? -1.0 : 1.0);
		return negative ? -0.0 : 0.0;
	}
	if (isinf(x)) {
		double result = n < 0 ? 0.0 : INFINITY;

		return negative ? -result : result;
	}
	if (fabs(x) == 1.0)
		return negative ? -1.0 : 1.0;
	if (n == 1)
		return x;

	return __libc_pow_integer(__libc_log_core(fabs(x)), n, negative);
}
