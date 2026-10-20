/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * catanhf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"
#include "internal/math.h"

/*
 * C23 7.3.6.3, G.6.2.3 (Kahan): on x >= 0 (catanh is odd), the real part
 * log1p(4x / ((1 - x)^2 + y^2))/4 and the imaginary part
 * atan2(2y, (1 - x)(1 + x) - y^2)/2, all in float-float (1 -+ x exact).
 * Where 4x/D or 2y/d is below 2^-60, the parts are the first-order terms x/D
 * and y/d (exact to 2^-60), formed from x or y scaled by 2^64, so that a
 * tiny quotient does not lose its low part to underflow. From 2^40 the
 * real part is Re(1/z) (relative error 2^-80) and the imaginary part +-pi/2
 * (2^-40); below, the squares stay under 2^82, where the float-float
 * products cannot overflow. At x = 1 with |y|
 * below 2^-60 the real part is (log 2 - log|y|)/2 (y^2 would underflow).
 * catanh(+-1 + i0) is +-infinity (divide-by-zero) (PLAN D67).
 */
float complex catanhf(float complex z)
{
	float x = __libc_ref(z);
	float y = __libc_imf(z);
	float a = fmaxf(fabsf(x), fabsf(y));
	struct __libc_ff denominator;
	struct __libc_ff difference;
	struct __libc_ff value;
	float real;
	float imag;
	int scale;

	if (isinf(x) || isinf(y))
		return CMPLXF(copysignf(0.0f, x), isnan(y) ? y : copysignf(0x1.921fb6p+0f, y));
	if (isnan(x) || isnan(y))
		return CMPLXF(x == 0 ? x : x + y, x + y);
	if (signbit(x))
		return -catanhf(CMPLXF(-x, -y));
	if (a >= 0x1p40f) {
		scale = ilogbf(a);
		value = __libc_ff_add(__libc_ff_product(scalbnf(x, -scale), scalbnf(x, -scale)), __libc_ff_product(scalbnf(y, -scale), scalbnf(y, -scale)));
		return CMPLXF(__libc_exp2f_scale(__libc_ff_divide((struct __libc_ff){ .hi = scalbnf(x, -scale), .lo = 0.0f }, value), -scale), copysignf(0x1.921fb6p+0f, y));
	}
	if (x == 1.0f && y == 0)
		return CMPLXF(x / fabsf(y), y);

	difference = __libc_ff_sum(1.0f, -x);
	denominator = __libc_ff_add(__libc_ff_multiply(difference, difference), __libc_ff_product(y, y));
	if (x == 1.0f && fabsf(y) < 0x1p-60f) {
		value = __libc_log2f_core(fabsf(y), 0.0f);
		value = __libc_ff_add((struct __libc_ff){ .hi = 1.0f, .lo = 0.0f }, (struct __libc_ff){ .hi = -value.hi, .lo = -value.lo });
		value = __libc_ff_multiply(value, __LIBC_LN2F);
		real = 0.5f * value.hi + 0.5f * value.lo;
	} else if (4.0f * x < 0x1p-60f * denominator.hi) {
		/* log1p(4x/D)/4 = x/D (1 - 2x/D + ...) */
		real = x == 0 ? x : __libc_exp2f_scale(__libc_ff_divide((struct __libc_ff){ .hi = scalbnf(x, 64), .lo = 0.0f }, denominator), -64);
	} else {
		value = __libc_ff_divide((struct __libc_ff){ .hi = 4.0f * x, .lo = 0.0f }, denominator);
		value = __libc_log1pf_core(value);
		real = 0.25f * value.hi + 0.25f * value.lo;
	}

	denominator = __libc_ff_add(__libc_ff_multiply(difference, __libc_ff_sum(1.0f, x)), (struct __libc_ff){ .hi = -__libc_ff_product(y, y).hi, .lo = -__libc_ff_product(y, y).lo });
	if (2.0f * fabsf(y) < 0x1p-60f * denominator.hi) {
		/* atan2(2y, d)/2 = y/d (1 - (2y/d)^2/3 + ...) */
		imag = y == 0 ? y : __libc_exp2f_scale(__libc_ff_divide((struct __libc_ff){ .hi = scalbnf(y, 64), .lo = 0.0f }, denominator), -64);
	} else {
		value = __libc_atan2f_core((struct __libc_ff){ .hi = 2.0f * y, .lo = 0.0f }, denominator);
		imag = 0.5f * value.hi + 0.5f * value.lo;
	}

	return CMPLXF(real, imag);
}
