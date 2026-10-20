/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * log10p1f.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.6.13: log2(1 + x) times log10 2, both as float-float (PLAN D58) */
float log10p1f(float x)
{
	struct __libc_ff value;

	if (isnan(x))
		return x + x;
	if (x < -1.0f)
		return __libc_math_invalidf();
	/* log(1 + -1) is -infinity with divide-by-zero */
	if (x == -1.0f)
		return -1.0f / fabsf(x + 1.0f);
	if (isinf(x))
		return x;
	/*
	 * Far below 1 the result is x c rounded once; x is scaled into [1, 2)
	 * first so that the exact product does not underflow, and the scaling
	 * back rounds once, subnormal results included.
	 */
	if (fabsf(x) < 0x1p-90f) {
		int shift;

		if (x == 0)
			return x;
		shift = ilogbf(x);
		return __libc_exp2f_scale(__libc_ff_scale(scalbnf(x, -shift), __LIBC_INV_LN10F), shift);
	}

	value = __libc_log2p1f_core(x);
	value = __libc_ff_multiply(value, __LIBC_LOG10_2F);

	return value.hi + value.lo;
}
