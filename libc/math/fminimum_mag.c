/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fminimum_mag.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.12.7: the argument of smaller magnitude, fminimum of equal ones */
double fminimum_mag(double x, double y)
{
	double ax = fabs(x);
	double ay = fabs(y);

	if (isnan(x) || isnan(y))
		return x + y;
	if (ax < ay)
		return x;
	if (ay < ax)
		return y;

	return fminimum(x, y);
}
