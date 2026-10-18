/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strrchr.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <string.h>

/* C23 7.26.5.6 */
char *(strrchr)(const char *s, int c)
{
	const char *last = nullptr;
	char ch = c;

	while (true) {
		if (*s == ch)
			last = s;
		if (*s == '\0')
			return (char *)last;
		++s;
	}
}
