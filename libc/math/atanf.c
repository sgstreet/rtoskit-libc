/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * atanf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.4.3: evaluated in double, then rounded once more; tiny arguments round by hand (docs/research/libm-trig.md) */
float atanf(float x)
{
	/* The arguments whose double result is a float halfway point while the exact value is not (PLAN D98) */
	static const struct __libc_float_case cases[] = {
		{ 0x3d8d6b23, 0x3d8d31c3 }, /* 0x1.1ad646p-4 -> 0x1.1a6386p-4 */
		{ 0xbd8d6b23, 0xbd8d31c3 }, /* -0x1.1ad646p-4 -> -0x1.1a6386p-4 */
	};

	if (isless(fabsf(x), 0x1p-30f))
		return __libc_math_tinyf(x, -x);

	return __libc_float_round(atan(x), x, cases, sizeof(cases) / sizeof(cases[0]));
}
