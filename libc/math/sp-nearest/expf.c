/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * expf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.6.1: e^x = 2^(x / ln 2), with x / ln 2 as float-float (an exact
 * product plus the low part of 1/ln 2), through the exp2 core (PLAN D57).
 */
float expf(float x)
{
	int exponent;
	struct __libc_ff value;

	if (isnan(x))
		return x + x;
	if (x > 89.0f)
		return x * 0x1p127f;
	if (x < -104.0f)
		return 0x1p-100f * 0x1p-100f;
	/* e^x rounds to 1 + x */
	if (fabsf(x) < 0x1p-25f)
		return 1.0f + x;

	value = __libc_exp2f_core(__libc_ff_scale(x, __LIBC_INV_LN2F), &exponent);

	return __libc_exp2f_scale(value, exponent);
}
