/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * hello-printf-float.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>

/* Reference program: printf with the floating-point conversions */
int main(int argc, char **argv)
{
	double value = argc / 3.0;

	return printf("%s %f %e %g\n", argv[0], value, value * 1e10, -value) < 0;
}
