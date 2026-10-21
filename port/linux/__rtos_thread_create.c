/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_thread_create.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>

#include "linux.h"

static unsigned int last_lock_token = __LINUX_MAIN_LOCK_TOKEN;

/*
 * The new thread's first function: record itself and take a lock token
 * (even and nonzero, unique for 2^31 threads), then run the entry
 */
static int thread_start(void *argument)
{
	struct __rtos_thread *thread = argument;

	__rtos_linux_self = thread;
	__rtos_lock_token = __atomic_add_fetch(&last_lock_token, 2, __ATOMIC_RELAXED);
	__rtos_thread_exit(thread->entry(thread->arg));
}

/*
 * One mapping per thread: the stack grows down from the thread record, and
 * the TLS block follows the record (docs/research/threads.md). The kernel
 * stores the tid before clone returns and clears it when the thread is gone.
 */
int __rtos_thread_create(struct __rtos_thread **result, int (*entry)(void *), void *arg)
{
	size_t tls_size = __libc_tls_block_size(&__rtos_linux_tls_image);
	size_t size = (__LINUX_STACK_SIZE + sizeof(struct __rtos_thread) + tls_size + __LINUX_PAGE_SIZE - 1) & ~(__LINUX_PAGE_SIZE - 1);
	struct __rtos_thread *thread;
	long mapping;
	long status;
	void *tp;

	__rtos_linux_reap();

	mapping = __linux_syscall6(__LINUX_SYS_MMAP, 0, (long)size, __LINUX_PROT_READ | __LINUX_PROT_WRITE, __LINUX_MAP_PRIVATE | __LINUX_MAP_ANONYMOUS, -1, 0);
	if (__linux_is_error(mapping))
		return -ENOMEM;

	thread = (struct __rtos_thread *)(mapping + __LINUX_STACK_SIZE);
	*thread = (struct __rtos_thread){ .tid = 0, .state = __LINUX_THREAD_JOINABLE, .entry = entry, .arg = arg, .mapping = (void *)mapping, .mapping_size = size };
	tp = __libc_tls_init(thread + 1, &__rtos_linux_tls_image);

	status = __rtos_linux_clone(__LINUX_CLONE_THREAD_FLAGS, thread, &thread->tid, &thread->tid, tp, thread_start, thread);
	if (__linux_is_error(status)) {
		__linux_syscall2(__LINUX_SYS_MUNMAP, mapping, (long)size);
		return (int)status;
	}

	*result = thread;
	return 0;
}
