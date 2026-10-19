/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * hypot.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.7.4, F.10.4.4: sqrt(x^2 + y^2) without spurious overflow or
 * underflow: scaled by 2^-600 or 2^600, the squares summed as double-
 * doubles, and the square root corrected by the residual (S - h^2)/(2h).
 * An infinity wins over a NaN.
 */
double hypot(double x, double y)
{
	double a = fabs(x);
	double b = fabs(y);
	int scale = 0;
	struct __libc_dd a2;
	struct __libc_dd b2;
	struct __libc_dd sum;
	struct __libc_dd h2;
	double h;

	if (isinf(x) || isinf(y))
		return INFINITY;
	if (isnan(x) || isnan(y))
		return x + y;
	if (a < b) {
		double swap = a;

		a = b;
		b = swap;
	}
	if (b == 0 || ilogb(a) - ilogb(b) > 60)
		return a + b;

	if (a > 0x1p500) {
		a *= 0x1p-600;
		b *= 0x1p-600;
		scale = 600;
	} else if (b < 0x1p-500) {
		a *= 0x1p600;
		b *= 0x1p600;
		scale = -600;
	}

	a2 = __libc_dd_product(a, a);
	b2 = __libc_dd_product(b, b);
	sum = __libc_dd_sum(a2.hi, b2.hi);
	sum.lo += a2.lo + b2.lo;
	h = sqrt(sum.hi + sum.lo);
	h2 = __libc_dd_product(h, h);
	h += (((sum.hi - h2.hi) - h2.lo) + sum.lo) / (2.0 * h);

	return scalbn(h, scale);
}
