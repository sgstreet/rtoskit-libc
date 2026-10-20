/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_u64_to_double.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/math.h"

/* The high half scaled by 2^32 and the low half are exact; the one addition rounds as the conversion would (PLAN D55) */
double __libc_u64_to_double(uint64_t x)
{
	return (double)(uint32_t)(x >> 32) * 0x1p32 + (double)(uint32_t)x;
}
