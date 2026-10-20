/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * erff.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.8.1: the positive series of the erf core below 3.92; beyond,
 * erfc(x) is below half an ulp of 1 and the result is +-1; far below 1,
 * 2x/sqrt(pi) rounded once (the x^3 term is under 2^-39) (PLAN D63).
 */
float erff(float x)
{
	float a = fabsf(x);
	struct __libc_ff value;
	int shift;

	if (isnan(x) || x == 0)
		return x + x;
	if (a >= 3.92f)
		return copysignf(1.0f, x);
	if (a < 0x1p-19f) {
		shift = ilogbf(x);
		return __libc_exp2f_scale(__libc_ff_scale(scalbnf(x, -shift), __LIBC_TWO_SQRTPIF), shift);
	}

	value = __libc_erff_core(a);

	return copysignf(value.hi + value.lo, x);
}
