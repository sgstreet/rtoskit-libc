/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * exp10m1f.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.6.3: evaluated in double, then rounded once more (docs/research/libm-exp-log.md) */
float exp10m1f(float x)
{
	/* The arguments whose double result is a float halfway point while the exact value is not (PLAN D98) */
	static const struct __libc_float_case cases[] = {
		{ 0x417d7f60, 0x59c64405 }, /* 0x1.fafecp+3 -> 0x1.8c880ap+52 */
	};

	return __libc_float_round(exp10m1(x), x, cases, sizeof(cases) / sizeof(cases[0]));
}
