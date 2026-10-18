/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strtok.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <string.h>

/* C23 7.26.5.9: where the next search starts, per thread (D6) */
static thread_local char *next;

char *strtok(char *restrict s1, const char *restrict s2)
{
	char *end;

	if (s1 == nullptr) {
		s1 = next;
		if (s1 == nullptr)
			return nullptr;
	}

	/* Skip leading delimiters; a string of only delimiters has no token */
	s1 += strspn(s1, s2);
	if (*s1 == '\0') {
		next = nullptr;
		return nullptr;
	}

	end = s1 + strcspn(s1, s2);
	if (*end != '\0') {
		*end = '\0';
		next = end + 1;
	} else {
		next = nullptr;
	}

	return s1;
}
