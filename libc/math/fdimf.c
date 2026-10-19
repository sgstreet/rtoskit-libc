/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fdimf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

/* C23 7.12.12.1: x - y if x > y, else +0; one rounding in the type itself */
float fdimf(float x, float y)
{
	if (isnan(x) || isnan(y))
		return x + y;

	return x > y ? x - y : 0;
}
