/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * memcmp.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <string.h>

/* C23 7.26.4.1: bytes compare as unsigned char */
int memcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *a = s1;
	const unsigned char *b = s2;

	for (; n > 0; --n, ++a, ++b)
		if (*a != *b)
			return *a - *b;

	return 0;
}
