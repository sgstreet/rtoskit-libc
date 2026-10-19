/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * roundeven.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

/* C23 7.12.9.8: VRINTN, to nearest with ties to even */
double roundeven(double x)
{
	double result;

	__asm__("vrintn.f64 %P0, %P1" : "=w"(result) : "w"(x));
	return result;
}
