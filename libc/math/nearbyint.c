/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * nearbyint.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <fenv.h>
#include <math.h>

/* C23 7.12.9.3: rint without the inexact exception */
double nearbyint(double x)
{
	fexcept_t flags;
	double result;

	if (isnan(x))
		return x + x;
	fegetexceptflag(&flags, FE_ALL_EXCEPT);
	result = rint(x);
	fesetexceptflag(&flags, FE_ALL_EXCEPT);

	return result;
}
