/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_lock_release.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "linux.h"

void __rtos_lock_release(struct __rtos_lock *lock)
{
	if (__atomic_load_n(&lock->owner, __ATOMIC_RELAXED) != __rtos_thread_self() || lock->depth == 0)
		__rtos_abort();

	if (--lock->depth != 0)
		return;

	__atomic_store_n(&lock->owner, nullptr, __ATOMIC_RELAXED);
	if (__atomic_exchange_n(&lock->state, 0, __ATOMIC_RELEASE) == 2)
		__rtos_futex_wake(&lock->state, 1);
}
