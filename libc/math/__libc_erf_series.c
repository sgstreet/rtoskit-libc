/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_erf_series.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * erf(x) = 2/sqrt(pi) (x - x^3/3 + x^5/10 - ...), the terms
 * (-1)^n x^(2n+1)/(n! (2n+1)), for |x| < 1/2: x - x^3/3 as double-doubles,
 * then n = 2 ... 13 (the next term is below 2^-62 relative).
 */
struct __libc_dd __libc_erf_series(double x)
{
	const struct __libc_dd third = { .hi = 0x1.5555555555555p-2, .lo = 0x1.5555555555555p-56 };
	const struct __libc_dd two_sqrtpi = { .hi = 0x1.20dd750429b6dp+0, .lo = 0x1.1ae3a914fed80p-56 };
	double z = x * x;
	struct __libc_dd cube = __libc_dd_multiply(__libc_dd_multiply(__libc_dd_product(x, x), (struct __libc_dd){ .hi = x, .lo = 0.0 }), third);
	double tail = 0.0;
	double factorial = 1.0;
	struct __libc_dd sum;

	for (int n = 2; n <= 13; ++n)
		factorial *= n;
	for (int n = 13; n >= 2; --n) {
		tail = tail * z + ((n & 1) != 0 ? -1.0 : 1.0) / (factorial * (2 * n + 1));
		factorial /= n;
	}
	sum = __libc_dd_fast_sum(x, -cube.hi);
	sum = __libc_dd_fast_sum(sum.hi, sum.lo - cube.lo + x * z * z * tail);

	return __libc_dd_multiply(sum, two_sqrtpi);
}
