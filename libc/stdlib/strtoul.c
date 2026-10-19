/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strtoul.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <limits.h>
#include <stdlib.h>

#include "internal/stdlib.h"

/* C23 7.24.1.7 */
unsigned long int strtoul(const char *restrict nptr, char **restrict endptr, int base)
{
	return (unsigned long int)__libc_strtoint(nptr, endptr, base, false, ULONG_MAX);
}
