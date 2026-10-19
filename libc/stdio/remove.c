/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * remove.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <stdio.h>

/* C23 7.23.4.1 */
int remove(const char *filename)
{
	int status = __rtos_remove(filename);

	if (status < 0) {
		errno = -status;
		return -1;
	}

	return 0;
}
