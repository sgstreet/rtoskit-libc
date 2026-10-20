/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * rintf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.9.4: adding and subtracting 2^23 (with the sign of x) rounds in
 * the current direction and raises inexact as the FPU does; the sign is put
 * back for the results that round to zero.
 */
float rintf(float x)
{
	float big = signbit(x) ? -0x1p23f : 0x1p23f;

	if (!isless(fabsf(x), 0x1p23f))
		return isnan(x) ? x + x : x;

	return copysignf((x + big) - big, x);
}
