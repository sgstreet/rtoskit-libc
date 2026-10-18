/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * memcpy.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <string.h>

/* C23 7.26.2.1 */
void *memcpy(void *restrict s1, const void *restrict s2, size_t n)
{
	unsigned char *dst = s1;
	const unsigned char *src = s2;

	while (n--)
		*dst++ = *src++;

	return s1;
}
