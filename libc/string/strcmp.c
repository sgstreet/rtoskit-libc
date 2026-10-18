/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strcmp.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <string.h>

/* C23 7.26.4.2: characters compare as unsigned char */
int strcmp(const char *s1, const char *s2)
{
	const unsigned char *a = (const unsigned char *)s1;
	const unsigned char *b = (const unsigned char *)s2;

	while (*a != '\0' && *a == *b) {
		++a;
		++b;
	}

	return *a - *b;
}
