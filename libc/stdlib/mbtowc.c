/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * mbtowc.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>

/* C23 7.24.7.2: the "C" locale maps every byte to the wide character of the same value */
int mbtowc(wchar_t *restrict pwc, const char *restrict s, size_t n)
{
	if (s == nullptr)
		return 0;
	if (n == 0)
		return -1;

	if (pwc != nullptr)
		*pwc = (unsigned char)*s;

	return *s != '\0';
}
