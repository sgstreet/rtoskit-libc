/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_tiny_merge.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "tiny.h"

/* Absorb the free chunks that follow chunk p; a used chunk or the sentinel stops it, and p keeps its used bit */
void __libc_tiny_merge(void *p)
{
	size_t *header = __libc_tiny_header(p);

	while (!(*__libc_tiny_header((char *)p + (*header & ~__LIBC_TINY_USED)) & __LIBC_TINY_USED))
		*header += *__libc_tiny_header((char *)p + (*header & ~__LIBC_TINY_USED));
}
