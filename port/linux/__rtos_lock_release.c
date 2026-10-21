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

/*
 * Releasing a lock the caller does not hold aborts, which makes unbalanced
 * locking visible in the tests. The last release clears the word and wakes
 * a waiter if it was marked slow.
 */
void __rtos_lock_release(struct __rtos_lock *lock)
{
	if ((__atomic_load_n(&lock->word, __ATOMIC_RELAXED) & ~__RTOS_LOCK_SLOW) != __rtos_lock_token)
		__rtos_abort();

	if (lock->depth != 0) {
		--lock->depth;
		return;
	}

	if ((__atomic_exchange_n(&lock->word, 0, __ATOMIC_RELEASE) & __RTOS_LOCK_SLOW) != 0)
		__rtos_futex_wake(&lock->word, 1);
}
