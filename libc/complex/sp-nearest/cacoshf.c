/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cacoshf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"

/* C23 7.3.6.1, G.6.2.1: from cacosf: the real part is |Im cacos z| and the imaginary part Re cacos z with the sign of Im z */
float complex cacoshf(float complex z)
{
	float complex w = cacosf(z);

	return CMPLXF(fabsf(__libc_imf(w)), copysignf(__libc_ref(w), __libc_imf(z)));
}
