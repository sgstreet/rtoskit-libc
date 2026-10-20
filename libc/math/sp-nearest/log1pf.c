/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * log1pf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.6.14: log2(1 + x) times ln 2, both as float-float (PLAN D58) */
float log1pf(float x)
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
	/* log(1 + x) = x - x^2/2 rounds to x */
	if (fabsf(x) < 0x1p-25f)
		return x;

	value = __libc_log2p1f_core(x);
	value = __libc_ff_multiply(value, __LIBC_LN2F);

	return value.hi + value.lo;
}
