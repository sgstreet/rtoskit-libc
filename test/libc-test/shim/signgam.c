/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * signgam.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "signgam.h"

/* signgam for libc-test's lgamma tests; the values come from the library (the parentheses skip the wrappers) */
int signgam;

/* The sign of gamma(x): negative on (-1, 0), (-3, -2), ... and at -0 */
static int sign_of_gamma(double x)
{
	if (isnan(x) || x > 0)
		return 1;
	if (x == 0)
		return signbit(x) ? -1 : 1;
	if (x == floor(x))
		return 1;

	return fmod(floor(x), 2.0) != 0 ? -1 : 1;
}

double __shim_lgamma(double x)
{
	signgam = sign_of_gamma(x);
	return (lgamma)(x);
}

float __shim_lgammaf(float x)
{
	signgam = sign_of_gamma(x);
	return (lgammaf)(x);
}

long double __shim_lgammal(long double x)
{
	signgam = sign_of_gamma((double)x);
	return (lgammal)(x);
}
