/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * stdc_first_leading_zero_ul.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdbit.h>

/* C23 7.18.7 */
unsigned int stdc_first_leading_zero_ul(unsigned long int value)
{
	return stdc_first_leading_zero(value);
}
