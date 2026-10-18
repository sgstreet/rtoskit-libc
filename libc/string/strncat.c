/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strncat.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <string.h>

/* C23 7.26.3.2: at most n characters of s2, always followed by a terminator */
char *strncat(char *restrict s1, const char *restrict s2, size_t n)
{
	char *dst = s1 + strlen(s1);
	const char *end = memchr(s2, '\0', n);
	size_t length = end != nullptr ? (size_t)(end - s2) : n;

	memcpy(dst, s2, length);
	dst[length] = '\0';

	return s1;
}
