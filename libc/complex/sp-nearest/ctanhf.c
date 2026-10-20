/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ctanhf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"
#include "internal/math.h"

/*
 * C23 7.3.6.6, G.6.2.6: tanh(x + iy) = (sinh x cosh x + i sin y cos y) /
 * (sinh^2 x + cos^2 y), a denominator of two squares that cannot cancel
 * (Kahan's (b r s + i t)/(1 + b s^2) rearranged), all float-float; a tiny
 * sinh x or sin y is scaled by 2^64 so its part keeps its relative accuracy.
 * From |x| = 16 the real part is +-1 (it differs from it by at most
 * 4 e^-2|x|, either side) and the imaginary part 4 sin y cos y e^-2|x|
 * (relative error 2 e^-2|x| < 2^-44) through the exp2 core, a zero y kept
 * (PLAN D65).
 */
float complex ctanhf(float complex z)
{
	float x = __libc_ref(z);
	float y = __libc_imf(z);
	struct __libc_ff sine;
	struct __libc_ff cosine;
	struct __libc_ff sinh;
	struct __libc_ff cosh;
	struct __libc_ff denominator;
	struct __libc_ff power;

	if (isinf(x))
		return CMPLXF(copysignf(1.0f, x), isfinite(y) ? copysignf(0.0f, sinf(y) * cosf(y)) : copysignf(0.0f, y));
	if (isnan(x))
		return CMPLXF(x, y == 0 ? y : x);
	if (!isfinite(y))
		return CMPLXF(x == 0 ? x : y - y, y - y);

	__libc_sincosf_core((struct __libc_ff){ .hi = y, .lo = 0.0f }, &sine, &cosine);
	if (fabsf(x) >= 16.0f) {
		if (y == 0)
			return CMPLXF(copysignf(1.0f, x), y);
		power = __libc_ff_scale(-2.0f * fminf(fabsf(x), 200.0f), __LIBC_INV_LN2F);
		power = __libc_ff_add(power, (struct __libc_ff){ .hi = 2.0f, .lo = 0.0f });
		return CMPLXF(copysignf(1.0f, x), __libc_exp2f_times(power, __libc_ff_multiply(sine, cosine)));
	}

	__libc_sinhcoshf_core(x, &sinh, &cosh);
	denominator = __libc_ff_add(__libc_ff_multiply(sinh, sinh), __libc_ff_multiply(cosine, cosine));

	return CMPLXF(__libc_ff_round_product(sinh, __libc_ff_divide(cosh, denominator), 0), __libc_ff_round_product(sine, __libc_ff_divide(cosine, denominator), 0));
}
