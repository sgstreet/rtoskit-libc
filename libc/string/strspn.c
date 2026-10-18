/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strspn.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <string.h>

/* A 256-bit membership set of the characters of chars */
static void build_set(unsigned char set[static 32], const char *chars)
{
	for (const unsigned char *p = (const unsigned char *)chars; *p != '\0'; ++p)
		set[*p >> 3] |= 1U << (*p & 7);
}

static bool in_set(const unsigned char set[static 32], unsigned char c)
{
	return (set[c >> 3] >> (c & 7)) & 1;
}

/* C23 7.26.5.7 */
size_t strspn(const char *s1, const char *s2)
{
	unsigned char set[32] = {};
	const unsigned char *p = (const unsigned char *)s1;

	build_set(set, s2);
	while (*p != '\0' && in_set(set, *p))
		++p;

	return p - (const unsigned char *)s1;
}
