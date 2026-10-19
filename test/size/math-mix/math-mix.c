/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * math-mix.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdlib.h>

/* Reference program: common double and float functions without stdio; the result is the exit status */
int main(int argc, char **argv)
{
	double x = argc * 0.75;
	float y = (float)argc * 0.5f;
	double sum;
	float sumf;

	sum = sin(x) + cos(x) + tan(x) + atan2(x, 2.0) + exp(x) + log(x) + pow(x, 1.5) + sqrt(x) + floor(x) + fmod(x, 0.3);
	sumf = sinf(y) + cosf(y) + expf(y) + logf(y) + powf(y, 2.5f) + sqrtf(y) + fabsf(y);

	return isfinite(sum) && isfinite(sumf) && argv[0] != nullptr ? EXIT_SUCCESS : EXIT_FAILURE;
}
