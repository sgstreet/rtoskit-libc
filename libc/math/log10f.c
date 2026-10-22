/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * log10f.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.6.12: evaluated in double, then rounded once more (docs/research/libm-exp-log.md) */
float log10f(float x)
{
	/* The arguments whose double result is a float halfway point while the exact value is not (PLAN D98) */
	static const struct __libc_float_case cases[] = {
		{ 0x0efeee7a, 0xc1e99d23 }, /* 0x1.fddcf4p-98 -> -0x1.d33a46p+4 */
	};

	return __libc_float_round(log10(x), x, cases, sizeof(cases) / sizeof(cases[0]));
}
