/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strdup.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>
#include <string.h>

/* C23 7.26.2.6 */
char *strdup(const char *s)
{
	size_t size = strlen(s) + 1;
	char *copy = malloc(size);

	if (copy != nullptr)
		memcpy(copy, s, size);

	return copy;
}
