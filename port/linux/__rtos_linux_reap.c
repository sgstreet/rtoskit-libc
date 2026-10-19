/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_linux_reap.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "linux.h"

struct __rtos_thread *__rtos_linux_zombies;

/* Unmap the exited detached threads the kernel is done with; put the others back */
void __rtos_linux_reap(void)
{
	struct __rtos_thread *thread = __atomic_exchange_n(&__rtos_linux_zombies, nullptr, __ATOMIC_ACQUIRE);

	while (thread != nullptr) {
		struct __rtos_thread *next = thread->next;

		if (__atomic_load_n(&thread->tid, __ATOMIC_ACQUIRE) == 0) {
			__linux_syscall2(__LINUX_SYS_MUNMAP, (long)thread->mapping, (long)thread->mapping_size);
		} else {
			thread->next = __atomic_load_n(&__rtos_linux_zombies, __ATOMIC_RELAXED);
			while (!__atomic_compare_exchange_n(&__rtos_linux_zombies, &thread->next, thread, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED))
				continue;
		}
		thread = next;
	}
}
