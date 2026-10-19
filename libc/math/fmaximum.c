/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fmaximum.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.12.4: IEEE maximum: any NaN gives a NaN; +0 is larger than -0 */
double fmaximum(double x, double y)
{
	if (isnan(x) || isnan(y))
		return x + y;
	if (x == y)
		return signbit(x) ? y : x;

	return x > y ? x : y;
}
