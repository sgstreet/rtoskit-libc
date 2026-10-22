/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * atanpif.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.4.10: evaluated in double, then rounded once more (docs/research/libm-trig.md) */
float atanpif(float x)
{
	/* The arguments whose double result is a float halfway point while the exact value is not (PLAN D98) */
	static const struct __libc_float_case cases[] = {
		{ 0x3f693531, 0x3e70d331 }, /* 0x1.d26a62p-1 -> 0x1.e1a662p-3 */
		{ 0xbf693531, 0xbe70d331 }, /* -0x1.d26a62p-1 -> -0x1.e1a662p-3 */
	};

	return __libc_float_round(atanpi(x), x, cases, sizeof(cases) / sizeof(cases[0]));
}
