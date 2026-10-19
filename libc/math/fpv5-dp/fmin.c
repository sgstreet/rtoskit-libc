/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fmin.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

/* C23 7.12.12.3: VMINNM is IEEE 754-2008 minNum, as the generic fmin */
double fmin(double x, double y)
{
	double result;

	__asm__("vminnm.f64 %P0, %P1, %P2" : "=w"(result) : "w"(x), "w"(y));
	return result;
}
