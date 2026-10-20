/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * math-float.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdlib.h>

/* Reference program: math-mix's float functions alone, without stdio; the result is the exit status (P2.05c) */
int main(int argc, char **argv)
{
	float y = (float)argc * 0.5f;
	float sum;

	sum = sinf(y) + cosf(y) + expf(y) + logf(y) + powf(y, 2.5f) + sqrtf(y) + fabsf(y);

	return isfinite(sum) && argv[0] != nullptr ? EXIT_SUCCESS : EXIT_FAILURE;
}
