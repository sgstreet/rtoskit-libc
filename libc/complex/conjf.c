/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * conjf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>

#include "internal/complex.h"

/* C23 7.3.9.4: the imaginary part negated (a sign flip, not arithmetic) */
float complex conjf(float complex z)
{
	return CMPLXF(__libc_ref(z), -__libc_imf(z));
}
