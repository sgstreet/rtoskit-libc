/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_rename.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "linux.h"

int __rtos_rename(const char *old_path, const char *new_path)
{
	return (int)__linux_syscall6(__LINUX_SYS_RENAMEAT, __LINUX_AT_FDCWD, (long)old_path, __LINUX_AT_FDCWD, (long)new_path, 0, 0);
}
