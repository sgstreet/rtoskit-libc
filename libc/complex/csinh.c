/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * csinh.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"

/*
 * C23 7.3.6.5, G.6.2.5: sinh x cos y + i cosh x sin y, the special cases
 * first; beyond |x| = 709 the factor e^|x|/2 is applied in two halves and
 * the sign of sinh x multiplies the real part (cos y keeps its own sign).
 */
double complex csinh(double complex z)
{
	double x = __libc_re(z);
	double y = __libc_im(z);
	double e;

	if (y == 0)
		return CMPLX(sinh(x), y);
	if (x == 0 && !isfinite(y))
		return CMPLX(x, y - y);
	if (isinf(x))
		return isfinite(y) ? CMPLX(x * cos(y), INFINITY * sin(y)) : CMPLX(x, y - y);
	if (isnan(x))
		return CMPLX(x, x);
	if (!isfinite(y))
		return CMPLX(y - y, y - y);
	if (fabs(x) > 709.0) {
		e = exp(0.5 * fabs(x));
		return CMPLX(copysign(1.0, x) * (0.5 * e * cos(y) * e), 0.5 * e * sin(y) * e);
	}

	return CMPLX(sinh(x) * cos(y), cosh(x) * sin(y));
}
