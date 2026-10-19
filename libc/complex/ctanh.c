/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ctanh.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"

/*
 * C23 7.3.6.6, G.6.2.6: Kahan's formula with t = tan y, b = 1 + t^2,
 * s = sinh x, r = cosh x: (b r s + i t) / (1 + b s^2) (t stays below 2^54
 * for any double y); beyond |x| = 22 the real part is +-1 and the imaginary
 * part 4 sin y cos y e^-2|x|.
 */
double complex ctanh(double complex z)
{
	double x = __libc_re(z);
	double y = __libc_im(z);
	double t;
	double b;
	double s;
	double r;
	double d;

	if (isinf(x))
		return CMPLX(copysign(1.0, x), isfinite(y) ? copysign(0.0, sin(y) * cos(y)) : copysign(0.0, y));
	if (isnan(x))
		return CMPLX(x, y == 0 ? y : x);
	if (!isfinite(y))
		return CMPLX(x == 0 ? x : y - y, y - y);
	if (fabs(x) > 22.0)
		return CMPLX(copysign(1.0, x), 4.0 * sin(y) * cos(y) * exp(-2.0 * fabs(x)));

	t = tan(y);
	b = 1.0 + t * t;
	s = sinh(x);
	r = sqrt(1.0 + s * s);
	d = 1.0 + b * s * s;

	return CMPLX(b * r * s / d, t / d);
}
