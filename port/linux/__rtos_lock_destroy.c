/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_lock_destroy.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "linux.h"

/* Destroying a held lock is a libc bug */
void __rtos_lock_destroy(struct __rtos_lock *lock)
{
	if (lock->depth != 0)
		__rtos_abort();

	lock->allocated = false;
}
