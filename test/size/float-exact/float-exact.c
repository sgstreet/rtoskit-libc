/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * float-exact.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdlib.h>

/* Reference program: the exact float functions (rounding, remainders, decomposition, conversions) without stdio (P2.05b) */
int main(int argc, char **argv)
{
	float x = (float)argc * 1.75f;
	float integral;
	int exponent;
	float sum;
	long n;

	sum = floorf(x) + ceilf(x) + roundf(x) + truncf(x) + rintf(x) + fmodf(x, 0.3f) + remainderf(x, 0.7f) + frexpf(x, &exponent) + ldexpf(x, 3) + modff(x, &integral) + integral;
	n = lroundf(x) + lrintf(x);

	return isfinite(sum) && n > 0 && exponent > 0 && argv[0] != nullptr ? EXIT_SUCCESS : EXIT_FAILURE;
}
