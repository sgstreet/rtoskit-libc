/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cacos.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"

/*
 * C23 7.3.5.1, G.6.1.1: Kahan's formula (Branch cuts for complex elementary
 * functions, 1987): with u = sqrt(1 - z) and v = sqrt(1 + z),
 * cacos(z) = 2 atan2(Re u, Re v) + i asinh(Im(conj(v) u)); 1 - z and 1 + z
 * are formed part by part so the signs of zero survive. Special cases first.
 */
double complex cacos(double complex z)
{
	double x = __libc_re(z);
	double y = __libc_im(z);
	double complex u;
	double complex v;
	double real;

	if (isinf(x) || isinf(y)) {
		if (isnan(x))
			return CMPLX(x, -y);
		if (isnan(y))
			return CMPLX(y, -x);
		if (isinf(x))
			real = x > 0 ? (isinf(y) ? 0x1.921fb54442d18p-1 : 0.0) : (isinf(y) ? 0x1.2d97c7f3321d2p+1 : 0x1.921fb54442d18p+1);
		else
			real = 0x1.921fb54442d18p+0;
		return CMPLX(real, -copysign(INFINITY, y));
	}
	if (isnan(x))
		return CMPLX(x, x);
	if (isnan(y))
		return CMPLX(x == 0 ? 0x1.921fb54442d18p+0 : y, y);

	u = csqrt(CMPLX(1.0 - x, -y));
	v = csqrt(CMPLX(1.0 + x, y));

	return CMPLX(2.0 * atan2(__libc_re(u), __libc_re(v)), asinh(__libc_re(v) * __libc_im(u) - __libc_im(v) * __libc_re(u)));
}
