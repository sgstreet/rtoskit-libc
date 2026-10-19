/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * imaxdiv.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <inttypes.h>

/* C23 7.8.2.2 */
imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom)
{
	return (imaxdiv_t){ .quot = numer / denom, .rem = numer % denom };
}
