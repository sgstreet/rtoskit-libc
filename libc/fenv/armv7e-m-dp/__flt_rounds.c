/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __flt_rounds.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <float.h>

/*
 * The FPSCR RMode field (bits 22-23) governs VFP arithmetic. Its encodings
 * 0..3 are nearest, toward +infinity, toward -infinity and toward zero;
 * FLT_ROUNDS uses 1, 2, 3 and 0 for the same modes (C23 5.2.5.3.3).
 */
int __flt_rounds(void)
{
	static const int flt_rounds[4] = { 1, 2, 3, 0 };
	unsigned int fpscr;

	__asm__ volatile("vmrs %0, fpscr" : "=r"(fpscr));

	return flt_rounds[(fpscr >> 22) & 3];
}
