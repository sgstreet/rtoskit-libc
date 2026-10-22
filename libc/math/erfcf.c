/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * erfcf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.8.2: evaluated in double, then rounded once more (docs/research/libm-special.md) */
float erfcf(float x)
{
	/* The arguments whose double result is a float halfway point while the exact value is not (PLAN D98) */
	static const struct __libc_float_case cases[] = {
		{ 0xb76c9f62, 0x3f800085 }, /* -0x1.d93ec4p-17 -> 0x1.00010ap+0 */
	};

	return __libc_float_round(erfc(x), x, cases, sizeof(cases) / sizeof(cases[0]));
}
