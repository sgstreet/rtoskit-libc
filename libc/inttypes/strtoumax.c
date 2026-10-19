/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strtoumax.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <inttypes.h>
#include <stdint.h>

#include "internal/stdlib.h"

/* C23 7.8.2.3 */
uintmax_t strtoumax(const char *restrict nptr, char **restrict endptr, int base)
{
	return __libc_strtoint(nptr, endptr, base, false, UINTMAX_MAX);
}
