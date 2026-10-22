/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * log2p1f.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.6.17: evaluated in double, then rounded once more (docs/research/libm-exp-log.md) */
float log2p1f(float x)
{
	/* The arguments whose double result is a float halfway point while the exact value is not (PLAN D98) */
	static const struct __libc_float_case cases[] = {
		{ 0x4ebd09e3, 0x41f48013 }, /* 0x1.7a13c6p+30 -> 0x1.e90026p+4 */
		{ 0x52928e33, 0x4218c7fd }, /* 0x1.251c66p+38 -> 0x1.318ffap+5 */
	};

	return __libc_float_round(log2p1(x), x, cases, sizeof(cases) / sizeof(cases[0]));
}
