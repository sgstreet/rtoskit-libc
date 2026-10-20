/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * malloc.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <stdlib.h>

#include "tiny.h"

/* C23 7.24.3.6; malloc(0) returns a unique minimum chunk (J.3.12) */
void *malloc(size_t size)
{
	size_t stride = __libc_tiny_request(size);
	void *p = nullptr;

	__rtos_lock_acquire(&__rtos_malloc_lock);
	__libc_tiny_init();
	if (stride != 0)
		p = __libc_tiny_allocate(stride);
	__rtos_lock_release(&__rtos_malloc_lock);

	if (p == nullptr)
		errno = ENOMEM;

	return p;
}
