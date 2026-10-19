/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * free.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>
#include <rtos/rtos-port.h>

#include "internal/malloc.h"
#include "tlsf.h"

/* C23 7.24.3.3 */
void free(void *ptr)
{
	if (ptr == nullptr)
		return;

	__rtos_lock_acquire(&__rtos_malloc_lock);
	__libc_tlsf_release(__libc_tlsf_get(), ptr);
	__rtos_lock_release(&__rtos_malloc_lock);
}
