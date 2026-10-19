/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_lock_acquire.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "linux.h"

/* The three-state futex mutex of docs/research/threads.md, made recursive by the owner and depth */
void __rtos_lock_acquire(struct __rtos_lock *lock)
{
	struct __rtos_thread *self = __rtos_thread_self();
	unsigned int state = 0;

	if (__atomic_load_n(&lock->owner, __ATOMIC_RELAXED) == self) {
		++lock->depth;
		return;
	}

	if (!__atomic_compare_exchange_n(&lock->state, &state, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
		while (__atomic_exchange_n(&lock->state, 2, __ATOMIC_ACQUIRE) != 0)
			__rtos_futex_wait(&lock->state, 2, nullptr);
	}

	__atomic_store_n(&lock->owner, self, __ATOMIC_RELAXED);
	lock->depth = 1;
}
