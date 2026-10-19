/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * atan2f.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

/*
 * C23 7.12.4.4: evaluated in double, then rounded once more. A tiny quotient
 * with x > 0 (atan2 is q - q^3/3) is nudged toward zero by 2^-40 relative, so
 * the conversion rounds in the current direction and raises underflow for a
 * subnormal result (docs/research/libm-trig.md).
 */
float atan2f(float y, float x)
{
	if (isgreater(x, 0.0f) && islessgreater(y, 0.0f) && isfinite(x) && isfinite(y) && ilogb(y) - ilogb(x) < -30) {
		double quotient = (double)y / x;

		return (float)(quotient - quotient * 0x1p-40);
	}

	return (float)atan2(y, x);
}
