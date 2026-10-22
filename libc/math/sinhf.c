/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * sinhf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.5.5: evaluated in double, then rounded once more; tiny arguments round by hand (docs/research/libm-trig.md) */
float sinhf(float x)
{
	/* The arguments whose double result is a float halfway point while the exact value is not (PLAN D98) */
	static const struct __libc_float_case cases[] = {
		{ 0xba1285ff, 0xba1285ff }, /* -0x1.250bfep-11 -> -0x1.250bfep-11 */
		{ 0x3a1285ff, 0x3a1285ff }, /* 0x1.250bfep-11 -> 0x1.250bfep-11 */
	};

	if (isless(fabsf(x), 0x1p-30f))
		return __libc_math_tinyf(x, x);

	return __libc_float_round(sinh(x), x, cases, sizeof(cases) / sizeof(cases[0]));
}
