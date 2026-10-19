/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * setlocale.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <locale.h>
#include <string.h>

/*
 * C23 7.11.1.1: only the "C" locale (PLAN §1). "" names the native
 * environment, which is "C" here, and so does "POSIX". A null locale queries.
 */
char *setlocale(int category, const char *locale)
{
	if (category < LC_ALL || category > LC_TIME)
		return nullptr;
	if (locale != nullptr && strcmp(locale, "C") != 0 && strcmp(locale, "POSIX") != 0 && strcmp(locale, "") != 0)
		return nullptr;

	return (char *)"C";
}
