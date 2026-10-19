/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_lock_create.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>

#include "linux.h"

static struct __rtos_lock pool[__LINUX_LOCK_POOL_SIZE];

/* Claim a free pool entry; claiming is atomic so threads may create locks concurrently */
int __rtos_lock_create(struct __rtos_lock **lock)
{
	for (unsigned int i = 0; i < __LINUX_LOCK_POOL_SIZE; ++i) {
		unsigned int allocated = 0;

		if (__atomic_compare_exchange_n(&pool[i].allocated, &allocated, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
			pool[i].state = 0;
			pool[i].depth = 0;
			pool[i].owner = nullptr;
			*lock = &pool[i];
			return 0;
		}
	}

	return -ENOMEM;
}
