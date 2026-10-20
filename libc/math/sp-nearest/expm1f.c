/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * expm1f.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.6.6: e^x - 1. Near 0 the series on r = x keeps the
 * relative accuracy; elsewhere the exp2 core's result minus 1 is a
 * float-float sum, rounded once (PLAN D57).
 */
float expm1f(float x)
{
	int exponent;
	struct __libc_ff value;

	if (isnan(x))
		return x + x;
	if (x > 89.0f)
		return x * 0x1p127f;
	if (x < -18.0f)
		return -1.0f + 0x1p-100f;
	/* The series' r^2 term is below half an ulp of r */
	if (fabsf(x) < 0x1p-25f)
		return x;
	if (fabsf(x) < 0x1.6p-7f) {
		value = __libc_ff_expm1_series((struct __libc_ff){ .hi = x, .lo = 0.0f });
		return value.hi + value.lo;
	}

	value = __libc_exp2f_core(__libc_ff_scale(x, __LIBC_INV_LN2F), &exponent);

	return __libc_exp2m1f_scale(value, exponent);
}
