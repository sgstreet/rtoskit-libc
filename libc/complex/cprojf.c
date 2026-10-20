/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cprojf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"

/* C23 7.3.9.5: an infinite part projects to (+infinity, +-0) with the sign of the imaginary part */
float complex cprojf(float complex z)
{
	if (isinf(__libc_ref(z)) || isinf(__libc_imf(z)))
		return CMPLXF(INFINITY, copysignf(0.0f, __libc_imf(z)));

	return z;
}
