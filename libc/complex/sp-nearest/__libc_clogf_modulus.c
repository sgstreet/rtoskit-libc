/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_clogf_modulus.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"
#include "internal/math.h"

/* log|x + iy| as float-float for finite x, y not both zero and |z| away from 1: log2 of the exact squares of the parts scaled by 2^-scale, plus scale, times ln 2 (PLAN D65, D67) */
struct __libc_ff __libc_clogf_modulus(float x, float y)
{
	int scale = ilogbf(fmaxf(fabsf(x), fabsf(y)));
	float a = scalbnf(x, -scale);
	float b = scalbnf(y, -scale);
	struct __libc_ff value = __libc_ff_add(__libc_ff_product(a, a), __libc_ff_product(b, b));

	value = __libc_log2f_core(value.hi, value.lo);
	value = __libc_ff_add((struct __libc_ff){ .hi = 0.5f * value.hi, .lo = 0.5f * value.lo }, (struct __libc_ff){ .hi = (float)scale, .lo = 0.0f });

	return __libc_ff_multiply(value, __LIBC_LN2F);
}
