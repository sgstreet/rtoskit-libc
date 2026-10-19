/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_exp_dd.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* exp(x) as a double-double for |x| <= 40 (the exponent stays small), rounding to nearest expected */
struct __libc_dd __libc_exp_dd(double x)
{
	int exponent;
	struct __libc_dd value = __libc_exp_core(x, 0.0, &exponent);

	return (struct __libc_dd){ .hi = scalbn(value.hi, exponent), .lo = scalbn(value.lo, exponent) };
}
