/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strcat.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <string.h>

/* C23 7.26.3.1 */
char *strcat(char *restrict s1, const char *restrict s2)
{
	strcpy(s1 + strlen(s1), s2);

	return s1;
}
