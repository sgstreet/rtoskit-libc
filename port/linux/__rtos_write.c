/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_write.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "linux.h"

/* Linux error numbers match <errno.h> (PLAN D20), so results pass through */
ptrdiff_t __rtos_write(int handle, const void *buffer, size_t count)
{
	return __linux_syscall3(__linux_sys_write, handle, (long)buffer, (long)count);
}
