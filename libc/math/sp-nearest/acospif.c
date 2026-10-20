/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * acospif.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.4.8: acos x times 1/pi as float-float; acospi of 1, -1 and 0 is 0, 1 and 1/2 exactly (PLAN D61) */
float acospif(float x)
{
	struct __libc_ff value;

	if (isnan(x))
		return x + x;
	if (isgreater(fabsf(x), 1.0f))
		return __libc_math_invalidf();
	if (x == 1.0f)
		return 0.0f;
	if (x == -1.0f)
		return 1.0f;
	if (x == 0)
		return 0.5f;

	value = __libc_atan2f_core(__libc_ff_sqrt(__libc_ff_multiply(__libc_ff_sum(1.0f, -x), __libc_ff_sum(1.0f, x))), (struct __libc_ff){ .hi = x, .lo = 0.0f });
	value = __libc_ff_multiply(value, __LIBC_INV_PIF);

	return value.hi + value.lo;
}
