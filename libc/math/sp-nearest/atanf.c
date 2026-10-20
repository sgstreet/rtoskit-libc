/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * atanf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.4.3: the atan2 core at (x, 1); x itself below 2^-12, pi/2 rounded beyond 2^26 (PLAN D61) */
float atanf(float x)
{
	struct __libc_ff value;

	if (isnan(x))
		return x + x;
	/* The x^3/3 term is below half an ulp of x */
	if (fabsf(x) < 0x1p-12f)
		return x;
	/* pi/2 - 1/x rounds to pi/2 */
	if (fabsf(x) > 0x1p26f)
		return copysignf(0x1.921fb6p0f, x);

	value = __libc_atan2f_core((struct __libc_ff){ .hi = x, .lo = 0.0f }, (struct __libc_ff){ .hi = 1.0f, .lo = 0.0f });

	return value.hi + value.lo;
}
