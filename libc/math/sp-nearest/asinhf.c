/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * asinhf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.5.2: the asinh core on the float argument (its odd series below
 * 1/8, log(|x| + sqrt(x^2 + 1)) through the log2 core up to 2^20, log 2|x|
 * beyond), rounded once; x itself below 2^-12, where x^3/6 is below half an
 * ulp (PLAN D62, D68).
 */
float asinhf(float x)
{
	struct __libc_ff value;

	if (isnan(x) || isinf(x))
		return x + x;
	if (fabsf(x) < 0x1p-12f)
		return x;

	value = __libc_asinhf_core((struct __libc_ff){ .hi = x, .lo = 0.0f });

	return value.hi + value.lo;
}
