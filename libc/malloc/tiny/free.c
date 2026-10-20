/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * free.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>
#include <stdlib.h>

#include "tiny.h"

/* C23 7.24.3.3: the chunk is marked free; the next allocation that walks past it merges it */
void free(void *ptr)
{
	if (ptr == nullptr)
		return;

	__rtos_lock_acquire(&__rtos_malloc_lock);
	*__libc_tiny_header(ptr) &= ~__LIBC_TINY_USED;
	__rtos_lock_release(&__rtos_malloc_lock);
}
