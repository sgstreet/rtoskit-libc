/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fminf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

/* C23 7.12.12.3: VMINNM is IEEE 754-2008 minNum, as the generic fmin */
float fminf(float x, float y)
{
	float result;

	__asm__("vminnm.f32 %0, %1, %2" : "=t"(result) : "t"(x), "t"(y));
	return result;
}
