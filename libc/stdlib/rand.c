/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * rand.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <stdlib.h>

#include "internal/stdlib.h"

/*
 * C23 7.24.2.1 with PCG32 (XSH-RR, O'Neill 2014): advance the 64-bit LCG
 * state, then permute the old state into 32 output bits, of which the top 31
 * are returned (RAND_MAX).
 */
int rand(void)
{
	uint64_t old = __libc_rand_state;
	uint32_t xorshifted = (uint32_t)(((old >> 18) ^ old) >> 27);
	unsigned int rotation = old >> 59;

	__libc_rand_state = old * __LIBC_RAND_MULTIPLIER + __LIBC_RAND_INCREMENT;

	return (int)(((xorshifted >> rotation) | (xorshifted << (-rotation & 31))) >> 1);
}
