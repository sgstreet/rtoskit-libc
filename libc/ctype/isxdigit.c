/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * isxdigit.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <ctype.h>

#include "internal/ctype.h"

/* C23 7.4.2.12 */
int isxdigit(int c)
{
	return __libc_isdigit(c) || ((unsigned int)c | 0x20) - 'a' < 6;
}
