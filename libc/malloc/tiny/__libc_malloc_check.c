/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_malloc_check.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "internal/malloc.h"
#include "tiny.h"

/* Walking from the first chunk reaches the sentinel through chunks of valid stride */
bool __libc_malloc_check(void)
{
	bool status = true;

	__rtos_lock_acquire(&__rtos_malloc_lock);
	for (char *p = __libc_tiny_heap.top.first; status && p != nullptr && __libc_tiny_stride(p) != 0; p += __libc_tiny_stride(p))
		status = __libc_tiny_stride(p) % __LIBC_TINY_ALIGN == 0 && __libc_tiny_stride(p) >= __LIBC_TINY_MIN_STRIDE;
	__rtos_lock_release(&__rtos_malloc_lock);

	return status;
}
