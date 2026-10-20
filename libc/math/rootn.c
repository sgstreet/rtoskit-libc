/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * rootn.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.7.8: x^(1/n) for a nonzero integer n; a negative x needs an odd
 * n. exp(log|x| / n) with the quotient as a double-double.
 */
double rootn(double x, long long n)
{
	bool odd = (n & 1) != 0;
	bool negative = signbit(x) && odd;
	double divisor = __libc_i64_to_double(n);
	struct __libc_dd log;
	struct __libc_dd product;
	struct __libc_dd value;
	double zh;
	double zl;
	int exponent;

	if (n == 0)
		return __libc_math_invalid();
	if (isnan(x))
		return x + x;
	if (signbit(x) && !odd && x != 0)
		return __libc_math_invalid();
	if (x == 0) {
		if (n < 0)
			return __libc_math_divbyzero(negative ? -1.0 : 1.0);
		return odd ? x : 0.0;
	}
	if (isinf(x)) {
		double result = n < 0 ? 0.0 : INFINITY;

		return negative ? -result : result;
	}
	if (n == 1)
		return x;

	log = __libc_log_core(fabs(x));
	zh = log.hi / divisor;
	product = __libc_dd_product(zh, divisor);
	zl = (((log.hi - product.hi) - product.lo) + log.lo) / divisor;
	value = __libc_exp_core(zh, zl, &exponent);

	return __libc_exp_scale(negative ? __libc_dd_negate(value) : value, exponent);
}
