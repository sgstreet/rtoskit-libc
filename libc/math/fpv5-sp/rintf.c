/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * rintf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

/* C23 7.12.9.4: VRINTX rounds in the FPSCR direction and raises inexact */
float rintf(float x)
{
	float result;

	__asm__("vrintx.f32 %0, %1" : "=t"(result) : "t"(x));
	return result;
}
