/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * iscntrl.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <ctype.h>

#include "internal/ctype.h"

/* C23 7.4.2.4; in the "C" locale, ASCII control characters: 0x00 to 0x1f and DEL */
int iscntrl(int c)
{
	return (unsigned int)c < 0x20 || c == 0x7f;
}
