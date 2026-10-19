/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * lldiv.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>

/* C23 7.24.6.2: the quotient truncates toward zero and quot * denom + rem == numer */
lldiv_t lldiv(long long int numer, long long int denom)
{
	return (lldiv_t){ .quot = numer / denom, .rem = numer % denom };
}
