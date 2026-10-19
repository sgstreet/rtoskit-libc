/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * wcstombs.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

/* C23 7.24.8.2: (size_t)-1 when a wide character has no "C" locale encoding */
size_t wcstombs(char *restrict s, const wchar_t *restrict pwcs, size_t n)
{
	size_t i;

	for (i = 0; i < n; ++i) {
		if (pwcs[i] < 0 || pwcs[i] > UCHAR_MAX) {
			errno = EILSEQ;
			return (size_t)-1;
		}
		s[i] = (char)pwcs[i];
		if (pwcs[i] == L'\0')
			return i;
	}

	return n;
}
