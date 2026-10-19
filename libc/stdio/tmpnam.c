/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tmpnam.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <stdio.h>

/* The next candidate, shared by all threads under __rtos_stdio_lock */
static unsigned int next_name;

/* Without an argument the result lives in a buffer of the calling thread (C23 7.23.4.4p3, PLAN D6) */
static thread_local char name[L_tmpnam];

/*
 * C23 7.23.4.4: a name no file has at the time of the call, from the port's
 * candidates; a name exists if it can be opened for reading.
 */
char *tmpnam(char *s)
{
	char *buffer = s != nullptr ? s : name;

	for (unsigned int attempt = 0; attempt < TMP_MAX; ++attempt) {
		unsigned int index;

		__rtos_lock_acquire(&__rtos_stdio_lock);
		index = next_name++;
		__rtos_lock_release(&__rtos_stdio_lock);

		if (__rtos_tmpname(buffer, L_tmpnam, index) < 0)
			return nullptr;

		int handle = __rtos_open(buffer, __RTOS_OPEN_READ);

		if (handle == -ENOENT)
			return buffer;
		if (handle < 0 && handle != -EACCES)
			return nullptr;
		if (handle >= 0)
			__rtos_close(handle);
	}

	return nullptr;
}
