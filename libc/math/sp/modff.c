/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * modff.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.6.18: the integral part through truncf; the fraction has the sign of x, +-0 for an infinity (F.10.3.18) */
float modff(float x, float *iptr)
{
	float integral = truncf(x);

	*iptr = integral;
	if (isnan(x))
		return integral;
	if (isinf(x))
		return copysignf(0.0f, x);

	return copysignf(x - integral, x);
}
