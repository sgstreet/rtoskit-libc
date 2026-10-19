/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * mbstowcs.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>

/* C23 7.24.8.1: at most n wide characters; the terminator is stored only if it fits, and not counted */
size_t mbstowcs(wchar_t *restrict pwcs, const char *restrict s, size_t n)
{
	size_t i;

	for (i = 0; i < n; ++i) {
		pwcs[i] = (unsigned char)s[i];
		if (s[i] == '\0')
			return i;
	}

	return n;
}
