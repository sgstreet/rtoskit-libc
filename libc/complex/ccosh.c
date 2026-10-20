/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ccosh.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"

/*
 * C23 7.3.6.4, G.6.2.4: cosh x cos y + i sinh x sin y, the special cases
 * first; beyond |x| = 709 the factor e^|x|/2 is applied in two halves and
 * the sign of sinh x multiplies the imaginary part (sin y keeps its own sign).
 */
double complex ccosh(double complex z)
{
	double x = __libc_re(z);
	double y = __libc_im(z);
	double e;

	if (y == 0)
		return CMPLX(cosh(x), isfinite(x) ? sinh(x) * y : copysign(0.0, x) * y);
	if (x == 0 && !isfinite(y))
		return CMPLX(y - y, x);
	if (isinf(x))
		return isfinite(y) ? CMPLX(INFINITY * cos(y), x * sin(y)) : CMPLX(x * x, y - y);
	if (isnan(x))
		return CMPLX(x, x);
	if (!isfinite(y))
		return CMPLX(y - y, y - y);
	if (fabs(x) > 709.0) {
		e = exp(0.5 * fabs(x));
		return CMPLX(0.5 * e * cos(y) * e, copysign(1.0, x) * (0.5 * e * sin(y) * e));
	}

	return CMPLX(cosh(x) * cos(y), sinh(x) * sin(y));
}
