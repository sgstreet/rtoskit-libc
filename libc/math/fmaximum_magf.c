/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fmaximum_magf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.12.6: the argument of larger magnitude, fmaximum of equal ones */
float fmaximum_magf(float x, float y)
{
	float ax = fabsf(x);
	float ay = fabsf(y);

	if (isnan(x) || isnan(y))
		return x + y;
	if (ax > ay)
		return x;
	if (ay > ax)
		return y;

	return fmaximumf(x, y);
}
