/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * casinh.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"

/*
 * C23 7.3.6.2, G.6.2.2: special cases first; then casinh(z) = -i asin(iz)
 * with Kahan's asin: for w = iz, u = sqrt(1 - w), v = sqrt(1 + w),
 * asin(w) = atan2(Re w, Re(u v)) + i asinh(Im(conj(u) v)).
 */
double complex casinh(double complex z)
{
	double x = __libc_re(z);
	double y = __libc_im(z);
	double wx;
	double wy;
	double complex u;
	double complex v;

	if (isinf(x) || isinf(y)) {
		if (isnan(x))
			return CMPLX(y, x);
		if (isnan(y))
			return CMPLX(x, y);
		return CMPLX(copysign(INFINITY, x), isinf(y) ? copysign(isinf(x) ? 0x1.921fb54442d18p-1 : 0x1.921fb54442d18p+0, y) : copysign(0.0, y));
	}
	if (isnan(x))
		return CMPLX(x, y == 0 ? y : x);
	if (isnan(y))
		return CMPLX(y, y);

	wx = -y;
	wy = x;
	u = csqrt(CMPLX(1.0 - wx, -wy));
	v = csqrt(CMPLX(1.0 + wx, wy));

	return __libc_times_minus_i(CMPLX(atan2(wx, __libc_re(u) * __libc_re(v) - __libc_im(u) * __libc_im(v)), asinh(__libc_re(u) * __libc_im(v) - __libc_im(u) * __libc_re(v))));
}
