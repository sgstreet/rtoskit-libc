/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * exp10f.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.6.2: 10^x = 2^(x log2 10), the product as float-float (PLAN D57) */
float exp10f(float x)
{
	int exponent;
	struct __libc_ff value;

	if (isnan(x))
		return x + x;
	if (x > 39.0f)
		return x * 0x1p127f;
	if (x < -46.0f)
		return 0x1p-100f * 0x1p-100f;
	/* 10^x rounds to 1 + x ln 10 */
	if (fabsf(x) < 0x1p-27f)
		return 1.0f + x * __LIBC_LN10F.hi;

	value = __libc_exp2f_core(__libc_ff_scale(x, __LIBC_LOG2_10F), &exponent);

	return __libc_exp2f_scale(value, exponent);
}
