/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * csqrtf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"
#include "internal/math.h"

/*
 * C23 7.3.8.3, G.6.4.2: Kahan's method, t = sqrt((|x| + |z|)/2), then
 * t + i y/(2t) for x >= 0 and |y|/(2t) + i copysign(t, y) otherwise, in
 * float-float on the parts scaled by an even power of two (no overflow or
 * loss to underflow), each part rounded once (PLAN D65). The other part
 * divides the significand of y, its exponent applied in the final scaling, so
 * a small y is not lost to underflow.
 */
float complex csqrtf(float complex z)
{
	float x = __libc_ref(z);
	float y = __libc_imf(z);
	struct __libc_ff modulus;
	struct __libc_ff t;
	float other = 0.0f;
	int half;
	int exponent;

	if (isinf(y))
		return CMPLXF(INFINITY, y);
	if (isnan(x))
		return CMPLXF(x, x + y);
	if (isinf(x)) {
		if (x > 0)
			return CMPLXF(x, isnan(y) ? y : copysignf(0.0f, y));
		return CMPLXF(isnan(y) ? y : 0.0f, copysignf(INFINITY, y));
	}
	if (isnan(y))
		return CMPLXF(y, y);
	if (x == 0 && y == 0)
		return CMPLXF(0.0f, y);

	half = ilogbf(fmaxf(fabsf(x), fabsf(y))) / 2;
	exponent = y == 0 ? 0 : ilogbf(y);
	if (y != 0)
		other = 0.5f * scalbnf(fabsf(y), -exponent);
	x = scalbnf(x, -2 * half);
	y = scalbnf(y, -2 * half);
	modulus = __libc_ff_sqrt(__libc_ff_add(__libc_ff_product(x, x), __libc_ff_product(y, y)));
	t = __libc_ff_add((struct __libc_ff){ .hi = fabsf(x), .lo = 0.0f }, modulus);
	t = __libc_ff_sqrt((struct __libc_ff){ .hi = 0.5f * t.hi, .lo = 0.5f * t.lo });
	other = __libc_exp2f_scale(__libc_ff_divide((struct __libc_ff){ .hi = other, .lo = 0.0f }, t), exponent - half);
	if (x >= 0)
		return CMPLXF(__libc_exp2f_scale(t, half), copysignf(other, y));

	return CMPLXF(other, copysignf(__libc_exp2f_scale(t, half), y));
}
