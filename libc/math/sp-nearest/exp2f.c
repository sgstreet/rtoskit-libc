/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * exp2f.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.6.4: through the exp2 core; an integer x gives 2^x exactly (PLAN D57) */
float exp2f(float x)
{
	int exponent;
	struct __libc_ff value;

	if (isnan(x))
		return x + x;
	if (x >= 128.0f)
		return x * 0x1p127f;
	if (x < -151.0f)
		return 0x1p-100f * 0x1p-100f;

	value = __libc_exp2f_core((struct __libc_ff){ .hi = x, .lo = 0.0f }, &exponent);

	return __libc_exp2f_scale(value, exponent);
}
