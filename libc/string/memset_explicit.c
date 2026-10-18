/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * memset_explicit.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <string.h>

/* C23 7.26.6.2: the stores must happen even when the object is never read again */
void *memset_explicit(void *s, int c, size_t n)
{
	memset(s, c, n);

	/* The compiler must assume the asm reads all memory through s, so the stores stay */
	__asm__ volatile("" : : "r"(s) : "memory");

	return s;
}
