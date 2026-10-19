/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * rint.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

/* C23 7.12.9.4: VRINTX rounds in the FPSCR direction and raises inexact */
double rint(double x)
{
	double result;

	__asm__("vrintx.f64 %P0, %P1" : "=w"(result) : "w"(x));
	return result;
}
