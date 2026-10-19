/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * clock.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <time.h>

/* C23 7.29.2.1: processor time in CLOCKS_PER_SEC units, or (clock_t)(-1) */
clock_t clock(void)
{
	struct timespec used;
	int status = __rtos_clock_gettime(TIME_ACTIVE, &used);

	if (status < 0) {
		errno = -status;
		return (clock_t)-1;
	}

	return used.tv_sec * CLOCKS_PER_SEC + used.tv_nsec / (1000000000 / CLOCKS_PER_SEC);
}
