/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * setenv.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

/* POSIX setenv: the entry is allocated and never freed, which is enough for the tests */
int setenv(const char *name, const char *value, int overwrite)
{
	size_t name_length = name == nullptr ? 0 : strlen(name);
	size_t value_length = strlen(value);
	char *entry;

	if (name_length == 0 || strchr(name, '=') != nullptr) {
		errno = EINVAL;
		return -1;
	}
	entry = malloc(name_length + value_length + 2);
	if (entry == nullptr)
		return -1;
	memcpy(entry, name, name_length);
	entry[name_length] = '=';
	memcpy(entry + name_length + 1, value, value_length + 1);

	return t_setenv(entry, name_length, overwrite);
}
