/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strndup.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>
#include <string.h>

/* C23 7.26.2.7: at most size characters; s need not be terminated within them */
char *strndup(const char *s, size_t size)
{
	const char *end = memchr(s, '\0', size);
	size_t length = end != nullptr ? (size_t)(end - s) : size;
	char *copy = malloc(length + 1);

	if (copy != nullptr) {
		memcpy(copy, s, length);
		copy[length] = '\0';
	}

	return copy;
}
