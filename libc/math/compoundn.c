/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * compoundn.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.7.2: (1 + x)^n for x >= -1: exp(n log1p(x)); compoundn(x, 0) is
 * 1, compoundn(-1, n) is +infinity with divide-by-zero for n < 0.
 */
double compoundn(double x, long long n)
{
	if (isnan(x))
		return n == 0 ? 1.0 : x + x;
	if (x < -1.0)
		return __libc_math_invalid();
	if (n == 0)
		return 1.0;
	if (x == -1.0)
		return n < 0 ? __libc_math_divbyzero(1.0) : 0.0;
	if (isinf(x))
		return n < 0 ? 0.0 : INFINITY;
	if (x == 0)
		return 1.0;
	if (n == 1)
		return 1.0 + x;
	if (fabs(x) < 0x1p-60)
		return 1.0 + n * x;

	return __libc_pow_integer(__libc_log1p_core(x), n, false);
}
