/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_getenv.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "linux.h"

/* Search the environment the kernel passed to the process for "name=" */
char *__rtos_getenv(const char *name)
{
	if (__rtos_linux_environ == nullptr || name == nullptr || *name == '\0')
		return nullptr;

	for (char **entry = __rtos_linux_environ; *entry != nullptr; ++entry) {
		const char *a = name;
		char *b = *entry;

		while (*a != '\0' && *a == *b) {
			++a;
			++b;
		}
		if (*a == '\0' && *b == '=')
			return b + 1;
	}

	return nullptr;
}
