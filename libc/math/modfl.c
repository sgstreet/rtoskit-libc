/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * modfl.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

/* long double is double (PLAN D7) */
long double modfl(long double x, long double *iptr)
{
	double integral;
	double fraction = modf(x, &integral);

	*iptr = integral;
	return fraction;
}
