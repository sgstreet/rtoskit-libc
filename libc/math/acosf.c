/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * acosf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.4.1: evaluated in double, then rounded once more (docs/research/libm-trig.md) */
float acosf(float x)
{
	/* The arguments whose double result is a float halfway point while the exact value is not (PLAN D98) */
	static const struct __libc_float_case cases[] = {
		{ 0x328885a3, 0x3fc90fdb }, /* 0x1.110b46p-26 -> 0x1.921fb6p+0 */
		{ 0x39826222, 0x3fc907b5 }, /* 0x1.04c444p-12 -> 0x1.920f6ap+0 */
	};

	return __libc_float_round(acos(x), x, cases, sizeof(cases) / sizeof(cases[0]));
}
