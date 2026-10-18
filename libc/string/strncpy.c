/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strncpy.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <string.h>

/* C23 7.26.2.5: s2 need not be terminated within n; the rest of s1 is padded with zeros */
char *strncpy(char *restrict s1, const char *restrict s2, size_t n)
{
	const char *end = memchr(s2, '\0', n);
	size_t length = end != nullptr ? (size_t)(end - s2) : n;

	memcpy(s1, s2, length);
	memset(s1 + length, '\0', n - length);

	return s1;
}
