/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * logf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.6.11: log2 x times ln 2, both as float-float (PLAN D58) */
float logf(float x)
{
	struct __libc_ff value;

	if (isnan(x))
		return x + x;
	if (x < 0)
		return __libc_math_invalidf();
	/* log(+-0) is -infinity with divide-by-zero */
	if (x == 0)
		return -1.0f / fabsf(x);
	if (isinf(x))
		return x;

	value = __libc_log2f_core(x, 0.0f);
	value = __libc_ff_multiply(value, __LIBC_LN2F);

	return value.hi + value.lo;
}
