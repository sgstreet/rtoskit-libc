/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * log10p1f.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.6.13: evaluated in double, then rounded once more (docs/research/libm-exp-log.md) */
float log10p1f(float x)
{
	/* The arguments whose double result is a float halfway point while the exact value is not (PLAN D98) */
	static const struct __libc_float_case cases[] = {
		{ 0xb051e173, 0xafb64ccf }, /* -0x1.a3c2e6p-31 -> -0x1.6c999ep-32 */
		{ 0x399a7c00, 0x390629e5 }, /* 0x1.34f8p-12 -> 0x1.0c53cap-13 */
	};

	return __libc_float_round(log10p1(x), x, cases, sizeof(cases) / sizeof(cases[0]));
}
