/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __flt_rounds.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <float.h>

/*
 * The MXCSR rounding control field (bits 13-14) governs SSE arithmetic, which
 * is all float, double and (with -mlong-double-64) long double use. Its
 * encodings 0..3 are nearest, down, up and toward zero; FLT_ROUNDS uses
 * 1, 3, 2 and 0 for the same modes (C23 5.2.5.3.3).
 */
int __flt_rounds(void)
{
	static const int flt_rounds[4] = { 1, 3, 2, 0 };
	unsigned int mxcsr;

	__asm__ volatile("stmxcsr %0" : "=m"(mxcsr));

	return flt_rounds[(mxcsr >> 13) & 3];
}
