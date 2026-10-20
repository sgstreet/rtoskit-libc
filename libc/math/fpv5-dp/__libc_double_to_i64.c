/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_double_to_i64.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/math.h"

/*
 * x = high * 2^32 + low with high = floor(x / 2^32) and low in [0, 2^32):
 * the scaling, the floor (VRINTM), the product and the difference are all
 * exact for an integral x, and each half converts exactly from 32 bits
 * (VCVT), so nothing is rounded and no exception is raised (PLAN D55).
 */
int64_t __libc_double_to_i64(double x)
{
	double high;
	double low;
	int32_t high_bits;
	uint32_t low_bits;

	__asm__("vrintm.f64 %P0, %P1" : "=w"(high) : "w"(x * 0x1p-32));
	low = x - high * 0x1p32;
	__asm__("vcvt.s32.f64 %0, %P1" : "=t"(high_bits) : "w"(high));
	__asm__("vcvt.u32.f64 %0, %P1" : "=t"(low_bits) : "w"(low));

	return (int64_t)(((uint64_t)(uint32_t)high_bits << 32) | low_bits);
}
