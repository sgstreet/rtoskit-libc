/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * isblank.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <ctype.h>

#include "internal/ctype.h"

/* C23 7.4.2.3; in the "C" locale, the standard blank characters are space and horizontal tab */
int isblank(int c)
{
	return c == ' ' || c == '\t';
}
