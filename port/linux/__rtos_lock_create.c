/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_lock_create.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>

#include "linux.h"

/* A fixed pool: the allocator cannot be used, since it takes a lock itself */
static struct __rtos_lock pool[__LINUX_LOCK_POOL_SIZE];

int __rtos_lock_create(struct __rtos_lock **lock)
{
	for (unsigned int i = 0; i < __LINUX_LOCK_POOL_SIZE; ++i) {
		if (!pool[i].allocated) {
			pool[i] = (struct __rtos_lock){ .depth = 0, .allocated = true };
			*lock = &pool[i];
			return 0;
		}
	}

	return -ENOMEM;
}
