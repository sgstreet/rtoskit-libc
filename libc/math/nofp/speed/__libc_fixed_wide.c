/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_fixed_wide.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "fixed.h"

/*
 * a b from four products of 16-bit halves, each within 32 bits (Armv6-M
 * multiplies 32 by 32 to 32); one copy for every fixed-point function, as
 * the speed budget asks (PLAN D86)
 */
uint64_t __libc_fixed_wide(uint32_t a, uint32_t b)
{
	uint32_t a1 = a >> 16;
	uint32_t a0 = a & 0xffff;
	uint32_t b1 = b >> 16;
	uint32_t b0 = b & 0xffff;
	uint32_t low = a0 * b0;
	uint32_t middle = a1 * b0;
	uint32_t cross = a0 * b1;
	uint32_t high = a1 * b1;

	/* middle + cross can carry out of 32 bits: add them in two steps */
	middle += low >> 16;
	middle += cross;
	if (middle < cross)
		high += 0x10000;

	return ((uint64_t)(high + (middle >> 16)) << 32) | ((middle << 16) | (low & 0xffff));
}
