/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ctanf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>

#include "internal/complex.h"

/* C23 7.3.5.6: ctan(z) = -i ctanh(iz) (G.6) */
float complex ctanf(float complex z)
{
	return __libc_times_minus_if(ctanhf(__libc_times_if(z)));
}
