/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * csqrt.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"

/*
 * C23 7.3.8.3, G.6.4.2: Kahan's method: t = sqrt((|x| + |z|)/2), then
 * t + i y/(2t) for x >= 0 and |y|/(2t) + i copysign(t, y) otherwise, on the
 * parts scaled by an even power of two (no overflow or loss to underflow).
 */
double complex csqrt(double complex z)
{
	double x = __libc_re(z);
	double y = __libc_im(z);
	double t;
	int half;

	if (isinf(y))
		return CMPLX(INFINITY, y);
	if (isnan(x))
		return CMPLX(x, x + y);
	if (isinf(x)) {
		if (x > 0)
			return CMPLX(x, isnan(y) ? y : copysign(0.0, y));
		return CMPLX(isnan(y) ? y : 0.0, copysign(INFINITY, y));
	}
	if (isnan(y))
		return CMPLX(y, y);
	if (x == 0 && y == 0)
		return CMPLX(0.0, y);

	half = ilogb(fmax(fabs(x), fabs(y))) / 2;
	x = scalbn(x, -2 * half);
	y = scalbn(y, -2 * half);
	t = sqrt(0.5 * (fabs(x) + hypot(x, y)));
	if (x >= 0)
		return CMPLX(scalbn(t, half), scalbn(y / (2.0 * t), half));

	return CMPLX(scalbn(fabs(y) / (2.0 * t), half), scalbn(copysign(t, y), half));
}
