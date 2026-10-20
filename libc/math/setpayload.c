/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * setpayload.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#define __STDC_WANT_IEC_60559_EXT__

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* F.10.13.2: a quiet NaN with payload pl, an integer in [0, 2^51); otherwise +0 and failure */
int setpayload(double *res, double pl)
{
	if (!(isgreaterequal(pl, 0) && isless(pl, 0x1p51)) || pl != trunc(pl)) {
		*res = 0;
		return 1;
	}

	*res = __libc_double_from(__LIBC_DOUBLE_EXPONENT | __LIBC_DOUBLE_QUIET | (uint64_t)__libc_double_to_i64(pl));
	return 0;
}
