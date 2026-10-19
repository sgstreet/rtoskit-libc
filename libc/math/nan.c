/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * nan.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.11.2: as strtod("NAN(n-char-sequence)"), which ignores the
 * sequence (docs/research/strtod.md): the default quiet NaN for any tag.
 */
double nan(const char *)
{
	return __libc_double_from(__LIBC_DOUBLE_EXPONENT | __LIBC_DOUBLE_QUIET);
}
