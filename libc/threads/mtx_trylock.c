/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * mtx_trylock.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <limits.h>
#include <rtos/rtos-port.h>
#include <threads.h>

#include "internal/threads.h"

/* C23 7.28.4.5 */
int mtx_trylock(mtx_t *mtx)
{
	thrd_t self = __rtos_thread_self();
	unsigned int state = __LIBC_MUTEX_UNLOCKED;

	if ((mtx->__type & mtx_recursive) != 0 && __atomic_load_n(&mtx->__owner, __ATOMIC_RELAXED) == self) {
		if (mtx->__count == UINT_MAX)
			return thrd_error;
		++mtx->__count;
		return thrd_success;
	}

	if (!__atomic_compare_exchange_n(&mtx->__state, &state, __LIBC_MUTEX_LOCKED, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
		return thrd_busy;

	__atomic_store_n(&mtx->__owner, self, __ATOMIC_RELAXED);
	mtx->__count = 1;

	return thrd_success;
}
