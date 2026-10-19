/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cacosh.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"

/*
 * C23 7.3.6.1, G.6.2.1: from cacos: the real part is |Im cacos z| and the
 * imaginary part Re cacos z with the sign of Im z.
 */
double complex cacosh(double complex z)
{
	double complex w = cacos(z);

	return CMPLX(fabs(__libc_im(w)), copysign(__libc_re(w), __libc_im(z)));
}
