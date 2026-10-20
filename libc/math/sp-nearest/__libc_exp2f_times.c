/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_exp2f_times.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * 2^z m rounded once, for finite z and |m| at most 1 and not tiny (a sine
 * or cosine of a float): beyond +-300 the result overflows or vanishes
 * whatever m is, so z is clamped there to keep the core's reduction in range
 * (PLAN D65).
 */
float __libc_exp2f_times(struct __libc_ff z, struct __libc_ff m)
{
	struct __libc_ff value;
	int exponent;

	if (z.hi > 300.0f)
		z = (struct __libc_ff){ .hi = 300.0f, .lo = 0.0f };
	if (z.hi < -300.0f)
		z = (struct __libc_ff){ .hi = -300.0f, .lo = 0.0f };

	value = __libc_exp2f_core(z, &exponent);

	return __libc_ff_round_product(value, m, exponent);
}
