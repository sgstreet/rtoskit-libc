/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strxfrm.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <string.h>

/*
 * C23 7.26.4.5: in the "C" locale the transformation is the identity. Nothing
 * is stored unless the whole result and its terminator fit; s1 may be null
 * when n is zero.
 */
size_t strxfrm(char *restrict s1, const char *restrict s2, size_t n)
{
	size_t length = strlen(s2);

	if (length < n)
		memcpy(s1, s2, length + 1);

	return length;
}
