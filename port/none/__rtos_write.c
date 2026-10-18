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

#include <errno.h>
#include <rtos/rtos-port.h>

#include "rtos-none.h"

ptrdiff_t __rtos_write(int handle, const void *buffer, size_t count)
{
	if (handle != 1 && handle != 2)
		return -EBADF;

	__rtos_none_output(buffer, count);

	return (ptrdiff_t)count;
}
