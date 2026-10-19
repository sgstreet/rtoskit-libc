/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * scalbnf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.6.19: as scalbn, with steps of 2^127 and 2^-102 (2^-126 * 2^24) */
float scalbnf(float x, int n)
{
	if (n > 127) {
		x *= 0x1p127f;
		n -= 127;
		if (n > 127) {
			x *= 0x1p127f;
			n -= 127;
			if (n > 127)
				n = 127;
		}
	} else if (n < -126) {
		x *= 0x1p-102f;
		n += 102;
		if (n < -126) {
			x *= 0x1p-102f;
			n += 102;
			if (n < -126)
				n = -126;
		}
	}

	return x * __libc_float_from((uint32_t)(n + __LIBC_FLOAT_BIAS) << 23);
}
