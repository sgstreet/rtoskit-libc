/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_heap_region.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "linux.h"

/* One region of reserved address space; untouched pages cost nothing */
bool __rtos_heap_region(unsigned int index, void **base, size_t *size)
{
	long region;

	if (index != 0)
		return false;

	region = __linux_syscall6(__LINUX_SYS_MMAP, 0, (long)__LINUX_HEAP_SIZE, __LINUX_PROT_READ | __LINUX_PROT_WRITE, __LINUX_MAP_PRIVATE | __LINUX_MAP_ANONYMOUS | __LINUX_MAP_NORESERVE, -1, 0);
	if (__linux_is_error(region))
		return false;

	*base = (void *)region;
	*size = __LINUX_HEAP_SIZE;
	return true;
}
