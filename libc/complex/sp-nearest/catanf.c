/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * catanf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>

#include "internal/complex.h"

/* C23 7.3.5.3: catan(z) = -i catanh(iz) (G.6) */
float complex catanf(float complex z)
{
	return __libc_times_minus_if(catanhf(__libc_times_if(z)));
}
