/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * scalblnf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

/* C23 7.12.6.19: any scale beyond +-3000 over- or underflows anyway */
float scalblnf(float x, long int n)
{
	return scalbnf(x, n > 3000 ? 3000 : n < -3000 ? -3000 :
													(int)n);
}
