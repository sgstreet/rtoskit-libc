/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fmaxf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.12.2: IEEE 754-2008 maxNum (as the Arm VMAXNM instruction): a
 * quiet NaN is missing data, a signaling NaN raises invalid and gives a
 * NaN; +0 is taken as larger than -0.
 */
float fmaxf(float x, float y)
{
	if (isnan(x) || isnan(y)) {
		if (issignaling(x) || issignaling(y))
			return x + y;
		return isnan(x) ? y : x;
	}
	if (x == y)
		return signbit(x) ? y : x;

	return x > y ? x : y;
}
