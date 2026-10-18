/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strchr.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <string.h>

#include "internal/string.h"

/* C23 7.26.5.3: the terminator is part of the string, so strchr(s, '\0') finds it */
char *(strchr)(const char *s, int c)
{
	char *p = __libc_strchrnul(s, c);

	return *p == (char)c ? p : nullptr;
}
