/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strfroml.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <stdlib.h>

/*
 * C23 7.24.1.3: the format is "%" with an optional precision and one of
 * a A e E f F g G; anything else is undefined, so snprintf does the work.
 */
int strfroml(char *restrict s, size_t n, const char *restrict format, long double fp)
{
	return snprintf(s, n, format, (double)fp);
}
