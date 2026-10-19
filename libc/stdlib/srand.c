/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * srand.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <stdlib.h>

#include "internal/stdlib.h"

/* C23 7.24.2.2: PCG's seeding, one step from zero, add the seed, one more step */
void srand(unsigned int seed)
{
	__libc_rand_state = (__LIBC_RAND_INCREMENT + seed) * __LIBC_RAND_MULTIPLIER + __LIBC_RAND_INCREMENT;
}
