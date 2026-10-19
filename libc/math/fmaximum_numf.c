/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fmaximum_numf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.12.8: IEEE maximumNumber: a NaN is missing data (a signaling one
 * raises invalid); two NaNs give a NaN; +0 is larger than -0.
 */
float fmaximum_numf(float x, float y)
{
	if (isnan(x) || isnan(y)) {
		if (isnan(x) && isnan(y))
			return x + y;
		if (issignaling(x) || issignaling(y))
			__libc_math_raise_invalid();
		return isnan(x) ? y : x;
	}

	return fmaximumf(x, y);
}
