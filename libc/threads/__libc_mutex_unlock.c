/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_mutex_unlock.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>
#include <threads.h>

#include "internal/threads.h"

/* Only the owner may unlock (7.28.4.6p2); the last unlock wakes one waiter if any may sleep */
int __libc_mutex_unlock(mtx_t *mtx)
{
	if (__atomic_load_n(&mtx->__owner, __ATOMIC_RELAXED) != __rtos_thread_self() || mtx->__count == 0)
		return thrd_error;
	if (--mtx->__count != 0)
		return thrd_success;

	__atomic_store_n(&mtx->__owner, nullptr, __ATOMIC_RELAXED);
	if (__atomic_exchange_n(&mtx->__state, __LIBC_MUTEX_UNLOCKED, __ATOMIC_RELEASE) == __LIBC_MUTEX_CONTENDED)
		__rtos_futex_wake(&mtx->__state, 1);

	return thrd_success;
}
