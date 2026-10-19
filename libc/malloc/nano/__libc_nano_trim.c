/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_nano_trim.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>

#include "nano.h"

/* Shrink the used chunk p to stride, releasing the tail when it can form a chunk */
void __libc_nano_trim(void *p, size_t stride)
{
	size_t size = *__libc_nano_header(p);
	void *tail;

	if (size - stride < __LIBC_NANO_MIN_STRIDE)
		return;

	*__libc_nano_header(p) = stride;
	tail = (char *)p + stride;
	*__libc_nano_header(tail) = size - stride;
	__libc_nano_release(tail);
}
