/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * clog.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"
#include "internal/math.h"

/*
 * C23 7.3.7.2, G.6.3.2: log|z| + i arg z. Near |z| = 1 the real part is
 * log1p(x^2 + y^2 - 1)/2 with the squares exact as double-doubles; elsewhere
 * log(hypot) of the parts scaled by a power of two (no overflow), plus the
 * scale times ln 2.
 */
double complex clog(double complex z)
{
	double x = __libc_re(z);
	double y = __libc_im(z);
	double a = fmax(fabs(x), fabs(y));
	double b = fmin(fabs(x), fabs(y));
	double real;
	int scale;

	if (isinf(x) || isinf(y))
		return CMPLX(INFINITY, atan2(y, x));
	if (isnan(x) || isnan(y))
		return CMPLX(x + y, x + y);
	if (a == 0)
		return CMPLX(__libc_math_divbyzero(-1.0), atan2(y, x));

	if (a >= 0.5 && a <= 2.0) {
		struct __libc_dd value;
		int mode = __libc_math_nearest();

		value = __libc_dd_add(__libc_dd_product(a, a), __libc_dd_product(b, b));
		value = __libc_log1p_dd(__libc_dd_add(value, (struct __libc_dd){ .hi = -1.0, .lo = 0.0 }));
		__libc_math_restore(mode);
		real = 0.5 * value.hi + 0.5 * value.lo;
	} else {
		scale = ilogb(a);
		real = log(hypot(scalbn(a, -scale), scalbn(b, -scale))) + scale * 0x1.62e42fefa39efp-1;
	}

	return CMPLX(real, atan2(y, x));
}
