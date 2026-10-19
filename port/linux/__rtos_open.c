/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_open.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "linux.h"

int __rtos_open(const char *path, unsigned int flags)
{
	int access = (flags & __RTOS_OPEN_READ) && (flags & __RTOS_OPEN_WRITE) ? __LINUX_O_RDWR : (flags & __RTOS_OPEN_WRITE) ? __LINUX_O_WRONLY :
																															__LINUX_O_RDONLY;
	int linux_flags = access | __LINUX_O_CLOEXEC | __LINUX_O_LARGEFILE;

	if (flags & __RTOS_OPEN_APPEND)
		linux_flags |= __LINUX_O_APPEND;
	if (flags & __RTOS_OPEN_CREATE)
		linux_flags |= __LINUX_O_CREAT;
	if (flags & __RTOS_OPEN_TRUNCATE)
		linux_flags |= __LINUX_O_TRUNC;
	if (flags & __RTOS_OPEN_EXCLUSIVE)
		linux_flags |= __LINUX_O_EXCL;

	long handle = __linux_syscall6(__LINUX_SYS_OPENAT, __LINUX_AT_FDCWD, (long)path, linux_flags, __LINUX_CREATE_MODE, 0, 0);

	/* A temporary file loses its name at once; the kernel frees it at the last close */
	if (!__linux_is_error(handle) && (flags & __RTOS_OPEN_TEMPORARY))
		__linux_syscall3(__LINUX_SYS_UNLINKAT, __LINUX_AT_FDCWD, (long)path, 0);

	return (int)handle;
}
