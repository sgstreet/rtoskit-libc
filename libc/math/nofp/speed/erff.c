/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * erff.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"
#include "libm-fixed.h"

/*
 * C23 7.12.8.1: the fixed-point erf for 2^-19 <= |x| < 3.92; the float-float
 * series (PLAN D63) takes the results the fixed-point one cannot round with
 * certainty (PLAN D88). Beyond 3.92 the result is +-1, and far below 1 it
 * is 2x/sqrt(pi) rounded once.
 */
float erff(float x)
{
	float a = fabsf(x);
	struct __libc_ff value;
	uint32_t result;
	uint64_t v;
	int exponent;
	int shift;

	if (isnan(x) || x == 0)
		return x + x;
	if (a >= 3.92f)
		return copysignf(1.0f, x);
	if (a < 0x1p-19f) {
		shift = ilogbf(x);
		return __libc_exp2f_scale(__libc_ff_scale(scalbnf(x, -shift), __LIBC_TWO_SQRTPIF), shift);
	}

	v = __libc_erff_fixed(__libc_float_bits(x), false, &exponent);
	if (v != 0 && __libc_fixed_round((uint32_t)(v >> 31), exponent, __LIBC_FIXED_ERF_ERROR, &result))
		return __libc_float_from(result | (__libc_float_bits(x) & 0x80000000u));

	value = __libc_erff_core(a);

	return copysignf(value.hi + value.lo, x);
}
