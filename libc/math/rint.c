/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * rint.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.9.4: in the current rounding direction, raising inexact when the
 * value changes (the comparisons are quiet: a NaN raises nothing). Adding and subtracting 2^52 (with x's sign) leaves no
 * fraction bits; the result keeps the sign of x (rint(-0.25) is -0, and in
 * downward rounding 2^52 - 2^52 would be -0).
 */
double rint(double x)
{
	double big = signbit(x) ? -0x1p52 : 0x1p52;

	if (!isless(fabs(x), 0x1p52))
		return isnan(x) ? x + x : x;

	return copysign((x + big) - big, x);
}
