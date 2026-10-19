/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * thrd_sleep.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <threads.h>

/* C23 7.28.5.7: 0 when the time elapsed, -1 when a signal interrupted, -2 on failure */
int thrd_sleep(const struct timespec *duration, struct timespec *remaining)
{
	int status = __rtos_thread_sleep(duration, remaining);

	if (status == 0)
		return 0;

	return status == -EINTR ? -1 : -2;
}
