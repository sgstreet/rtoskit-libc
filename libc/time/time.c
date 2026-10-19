/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * time.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <time.h>

/* C23 7.29.2.4: (time_t)(-1) when no calendar time is available */
time_t time(time_t *timer)
{
	struct timespec now;
	int status = __rtos_clock_gettime(TIME_UTC, &now);
	time_t result = status < 0 ? (time_t)-1 : now.tv_sec;

	if (status < 0)
		errno = -status;
	if (timer != nullptr)
		*timer = result;

	return result;
}
