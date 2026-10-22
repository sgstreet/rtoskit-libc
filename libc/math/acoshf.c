/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * acoshf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.5.1: evaluated in double, then rounded once more (docs/research/libm-trig.md) */
float acoshf(float x)
{
	/* The arguments whose double result is a float halfway point while the exact value is not (PLAN D98) */
	static const struct __libc_float_case cases[] = {
		{ 0x655890d3, 0x4254d1f9 }, /* 0x1.b121a6p+75 -> 0x1.a9a3f2p+5 */
		{ 0x6eb1a8ec, 0x42845a89 }, /* 0x1.6351d8p+94 -> 0x1.08b512p+6 */
	};

	return __libc_float_round(acosh(x), x, cases, sizeof(cases) / sizeof(cases[0]));
}
