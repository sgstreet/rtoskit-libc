/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_thread_detach.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>

#include "linux.h"

/* A thread that already exited goes to the zombie list now; one still running puts itself there */
int __rtos_thread_detach(struct __rtos_thread *thread)
{
	unsigned int previous = __atomic_exchange_n(&thread->state, __LINUX_THREAD_DETACHED, __ATOMIC_ACQ_REL);

	if (previous == __LINUX_THREAD_DETACHED)
		return -EINVAL;
	if (previous == __LINUX_THREAD_EXITED && thread != &__rtos_linux_main_thread) {
		thread->next = __atomic_load_n(&__rtos_linux_zombies, __ATOMIC_RELAXED);
		while (!__atomic_compare_exchange_n(&__rtos_linux_zombies, &thread->next, thread, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED))
			continue;
		__rtos_linux_reap();
	}

	return 0;
}
