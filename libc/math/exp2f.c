/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * exp2f.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.6.4: evaluated in double, then rounded once more (docs/research/libm-exp-log.md) */
float exp2f(float x)
{
	/* The arguments whose double result is a float halfway point while the exact value is not (PLAN D98) */
	static const struct __libc_float_case cases[] = {
		{ 0x3b429d37, 0x3f804385 }, /* 0x1.853a6ep-9 -> 0x1.00870ap+0 */
		{ 0xbcf3a937, 0x3f7ac6b1 }, /* -0x1.e7526ep-6 -> 0x1.f58d62p-1 */
	};

	return __libc_float_round(exp2(x), x, cases, sizeof(cases) / sizeof(cases[0]));
}
