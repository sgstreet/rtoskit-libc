/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * memccpy.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <string.h>

/* C23 7.26.2.2 */
void *memccpy(void *restrict s1, const void *restrict s2, int c, size_t n)
{
	unsigned char *dst = s1;
	const unsigned char *src = s2;
	unsigned char byte = c;
	size_t i;

	for (i = 0; i < n; ++i) {
		dst[i] = src[i];
		if (dst[i] == byte)
			return dst + i + 1;
	}

	return nullptr;
}
