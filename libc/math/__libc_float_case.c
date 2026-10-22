/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_float_case.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * A hard case returns its correctly rounded result nudged by 2^-30 of itself
 * toward the exact value: to nearest that is the result, and in a directed
 * mode the addition rounds to the result or its neighbour as the exact value
 * would, raising inexact (PLAN D98, docs/research/float-hard-cases.md). The
 * exact value lies between the result and the halfway point y sits on (or
 * within a few double ulps of), so y gives the side.
 */
float __libc_float_case(double y, float x, const struct __libc_float_case *cases, size_t count)
{
	uint32_t bits = __libc_float_bits(x);

	for (size_t i = 0; i < count; ++i) {
		if (cases[i].x == bits) {
			float result = __libc_float_from(cases[i].result);
			float nudge = result * 0x1p-30f;

			return result + (((double)result < y) == (result > 0) ? nudge : -nudge);
		}
	}

	return (float)y;
}
