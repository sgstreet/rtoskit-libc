/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * hypotf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

/* C23 7.12.7.4: the squares of floats are exact in double, which cannot overflow here */
float hypotf(float x, float y)
{
	double a = x;
	double b = y;

	if (isinf(x) || isinf(y))
		return INFINITY;
	if (isnan(x) || isnan(y))
		return x + y;

	return (float)sqrt(a * a + b * b);
}
