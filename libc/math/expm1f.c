/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * expm1f.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.6.6: evaluated in double, then rounded once more (docs/research/libm-exp-log.md) */
float expm1f(float x)
{
	if (isless(fabsf(x), 0x1p-30f))
		return __libc_math_tinyf(x, 1.0);

	return (float)expm1(x);
}
