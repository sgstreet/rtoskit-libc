/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cproj.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"

/* C23 7.3.9.5: every infinity projects to +infinity + i copysign(0, imaginary part) */
double complex cproj(double complex z)
{
	if (isinf(__libc_re(z)) || isinf(__libc_im(z)))
		return CMPLX(INFINITY, copysign(0.0, __libc_im(z)));

	return z;
}
