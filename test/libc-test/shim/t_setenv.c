/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * t_setenv.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include "rtos-linux.h"

#include "test.h"

/* Room for the entries the tests add; the kernel's environment is copied in first */
#define ENVIRONMENT_MAX 64

static char *environment[ENVIRONMENT_MAX + 1];

int t_setenv(char *entry, size_t name_length, int overwrite)
{
	size_t count = 0;

	/* Take over the port's environment the first time */
	if (__rtos_linux_environ != environment) {
		for (char **e = __rtos_linux_environ; e != nullptr && *e != nullptr && count < ENVIRONMENT_MAX; ++e)
			environment[count++] = *e;
		environment[count] = nullptr;
		__rtos_linux_environ = environment;
	}

	for (count = 0; environment[count] != nullptr; ++count) {
		if (strncmp(environment[count], entry, name_length + 1) == 0) {
			if (overwrite)
				environment[count] = entry;
			return 0;
		}
	}
	if (count == ENVIRONMENT_MAX) {
		errno = ENOMEM;
		return -1;
	}
	environment[count] = entry;
	environment[count + 1] = nullptr;

	return 0;
}
