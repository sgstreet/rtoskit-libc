/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * call_once.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <limits.h>
#include <rtos/rtos-port.h>
#include <threads.h>

/* once_flag states (docs/research/threads.md) */
#define NOT_RUN 0u
#define RUNNING 1u
#define WAITING 2u
#define DONE 3u

/*
 * C23 7.28.2.1: the caller that moves the flag from NOT_RUN to RUNNING runs
 * func; the others sleep until DONE, which the release store publishes.
 */
void call_once(once_flag *flag, void (*func)(void))
{
	unsigned int state = __atomic_load_n(&flag->__state, __ATOMIC_ACQUIRE);

	if (state == DONE)
		return;

	state = NOT_RUN;
	if (__atomic_compare_exchange_n(&flag->__state, &state, RUNNING, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)) {
		func();
		if (__atomic_exchange_n(&flag->__state, DONE, __ATOMIC_RELEASE) == WAITING)
			__rtos_futex_wake(&flag->__state, UINT_MAX);
		return;
	}

	while (state != DONE) {
		if (state == RUNNING) {
			if (!__atomic_compare_exchange_n(&flag->__state, &state, WAITING, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
				continue;
			state = WAITING;
		}
		__rtos_futex_wait(&flag->__state, WAITING, nullptr);
		state = __atomic_load_n(&flag->__state, __ATOMIC_ACQUIRE);
	}
}
