/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * setpayloadsigf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#define __STDC_WANT_IEC_60559_EXT__

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* F.10.13.3: a signaling NaN with payload pl, an integer in [1, 2^22) (0 would encode infinity) */
int setpayloadsigf(float *res, float pl)
{
	if (!(isgreaterequal(pl, 1) && isless(pl, 0x1p22f)) || pl != truncf(pl)) {
		*res = 0;
		return 1;
	}

	*res = __libc_float_from(__LIBC_FLOAT_EXPONENT | (uint32_t)pl);
	return 0;
}
