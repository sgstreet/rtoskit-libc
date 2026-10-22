/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * lgammaf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.8.3: evaluated in double, then rounded once more (docs/research/libm-special.md) */
float lgammaf(float x)
{
	/* The arguments whose double result is a float halfway point while the exact value is not (PLAN D98) */
	static const struct __libc_float_case cases[] = {
		{ 0xb0d6f2ca, 0x41a23559 }, /* -0x1.ade594p-30 -> 0x1.446ab2p+4 */
		{ 0x3b7c53aa, 0x40b1d661 }, /* 0x1.f8a754p-9 -> 0x1.63acc2p+2 */
	};

	return __libc_float_round(lgamma(x), x, cases, sizeof(cases) / sizeof(cases[0]));
}
