/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * coshf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.5.4: (e^|x| + e^-|x|)/2 from the exp2 core with one rounding (PLAN D62) */
float coshf(float x)
{
	struct __libc_ff value;
	struct __libc_ff inverse;
	int exponent;

	if (isnan(x))
		return x + x;
	if (isinf(x))
		return fabsf(x);
	if (fabsf(x) > 90.0f)
		return 0x1p127f * 0x1p127f;

	value = __libc_expf_pair(x, &exponent, &inverse);

	return __libc_exp2f_scale(__libc_ff_add(value, inverse), exponent - 1);
}
