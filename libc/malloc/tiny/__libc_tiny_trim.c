/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_tiny_trim.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "tiny.h"

/* Shrink the used chunk p to stride, leaving the tail as a free chunk when it can form one */
void __libc_tiny_trim(void *p, size_t stride)
{
	size_t size = __libc_tiny_stride(p);

	if (size - stride < __LIBC_TINY_MIN_STRIDE)
		return;

	*__libc_tiny_header(p) = stride | __LIBC_TINY_USED;
	*__libc_tiny_header((char *)p + stride) = size - stride;
}
