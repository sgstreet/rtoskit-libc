/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * lgammaf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.8.3: the lgamma core rounded once; the poles (0 and the negative integers) are +infinity with divide-by-zero (PLAN D64) */
float lgammaf(float x)
{
	struct __libc_ff value;
	int sign;

	if (isnan(x))
		return x + x;
	if (isinf(x))
		return fabsf(x);
	if (x <= 0 && truncf(x) == x)
		return 1.0f / fabsf(x - x);
	if (x == 1.0f || x == 2.0f)
		return 0.0f;

	value = __libc_lgammaf_core(x, &sign);

	return value.hi + value.lo;
}
