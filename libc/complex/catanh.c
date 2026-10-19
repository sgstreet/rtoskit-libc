/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * catanh.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"
#include "internal/math.h"

/*
 * C23 7.3.6.3, G.6.2.3 (Kahan): the real part log1p(4x / ((1 - x)^2 + y^2))/4
 * and the imaginary part atan2(2y, (1 - x)(1 + x) - y^2)/2, the second
 * denominator as a double-double; beyond 2^500 the real part is Re(1/z) and
 * the imaginary part +-pi/2. catanh(+-1 + i0) is +-infinity (divide-by-zero).
 */
double complex catanh(double complex z)
{
	double x = __libc_re(z);
	double y = __libc_im(z);
	double a = fmax(fabs(x), fabs(y));
	struct __libc_dd denominator;
	double real;
	int mode;

	if (isinf(x) || isinf(y))
		return CMPLX(copysign(0.0, x), isnan(y) ? y : copysign(0x1.921fb54442d18p+0, y));
	if (isnan(x) || isnan(y))
		return CMPLX(x == 0 ? x : x + y, x + y);
	/* catanh is odd: the formula on x >= 0 keeps the argument of log1p non-negative (no cancellation near -1) */
	if (signbit(x))
		return -catanh(CMPLX(-x, -y));
	if (a > 0x1p500) {
		int scale = ilogb(a);
		double xs = scalbn(x, -scale);
		double ys = scalbn(y, -scale);

		return CMPLX(scalbn(xs / (xs * xs + ys * ys), -scale), copysign(0x1.921fb54442d18p+0, y));
	}
	if (fabs(x) == 1.0 && y == 0)
		return CMPLX(__libc_math_divbyzero(x), y);

	real = 0.25 * log1p(4.0 * x / ((1.0 - x) * (1.0 - x) + y * y));
	mode = __libc_math_nearest();
	denominator = __libc_dd_multiply(__libc_dd_sum(1.0, -x), __libc_dd_sum(1.0, x));
	denominator = __libc_dd_add(denominator, __libc_dd_negate(__libc_dd_product(y, y)));
	__libc_math_restore(mode);

	return CMPLX(real, 0.5 * atan2(2.0 * y, denominator.hi + denominator.lo));
}
