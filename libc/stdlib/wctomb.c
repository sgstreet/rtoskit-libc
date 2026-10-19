/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * wctomb.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

/* C23 7.24.7.3: only wide characters 0 to UCHAR_MAX have a "C" locale encoding */
int wctomb(char *s, wchar_t wc)
{
	if (s == nullptr)
		return 0;

	if (wc < 0 || wc > UCHAR_MAX) {
		errno = EILSEQ;
		return -1;
	}

	*s = (char)wc;
	return 1;
}
