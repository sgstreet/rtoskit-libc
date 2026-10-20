/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cabsf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"

/* C23 7.3.8.1, G.6: hypot of the parts (cabs(+-infinity + i NaN) = +infinity through hypotf) */
float cabsf(float complex z)
{
	return hypotf(__libc_ref(z), __libc_imf(z));
}
