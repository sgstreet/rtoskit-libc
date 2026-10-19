/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cexp.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"

/*
 * C23 7.3.7.1, G.6.3.1: e^x (cos y + i sin y); e^x is applied as two halves
 * beyond 709 so a finite result past exp's overflow threshold survives.
 */
double complex cexp(double complex z)
{
	double x = __libc_re(z);
	double y = __libc_im(z);
	double e;

	if (y == 0)
		return CMPLX(exp(x), y);
	if (isinf(x)) {
		if (x < 0)
			return isfinite(y) ? CMPLX(0.0 * cos(y), 0.0 * sin(y)) : CMPLX(0.0, 0.0);
		return isfinite(y) ? CMPLX(x * cos(y), x * sin(y)) : CMPLX(x, y - y);
	}
	if (isnan(x))
		return CMPLX(x, x);
	if (!isfinite(y))
		return CMPLX(y - y, y - y);
	if (x > 709.0) {
		e = exp(0.5 * x);
		return CMPLX(e * cos(y) * e, e * sin(y) * e);
	}

	e = exp(x);
	return CMPLX(e * cos(y), e * sin(y));
}
