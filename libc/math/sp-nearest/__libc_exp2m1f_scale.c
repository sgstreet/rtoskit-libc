/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_exp2m1f_scale.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * 2^exponent v - 1 = 2^exponent (v - 2^-exponent): the subtraction is a
 * float-float sum (2^-exponent is a float for exponent in [-27, 128]), the
 * result is rounded once and scaled exactly, being normal (PLAN D57).
 */
float __libc_exp2m1f_scale(struct __libc_ff v, int exponent)
{
	struct __libc_ff difference = __libc_ff_add(v, (struct __libc_ff){ .hi = -scalbnf(1.0f, -exponent), .lo = 0.0f });

	return scalbnf(difference.hi + difference.lo, exponent);
}
