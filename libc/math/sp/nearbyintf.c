/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * nearbyintf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <fenv.h>
#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.9.3: rintf with the exception flags restored, so inexact is not raised */
float nearbyintf(float x)
{
	fexcept_t flags;
	float result;

	if (isnan(x))
		return x + x;
	fegetexceptflag(&flags, FE_ALL_EXCEPT);
	result = rintf(x);
	fesetexceptflag(&flags, FE_ALL_EXCEPT);

	return result;
}
