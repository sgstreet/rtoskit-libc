/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __flt_rounds.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <float.h>

#include "fenv-arch.h"

/* C23 5.2.5.3.3p8: -1 indeterminable, 0 toward zero, 1 nearest, 2 upward, 3 downward */
int __flt_rounds(void)
{
	static const int flt_rounds[4] = { 1, 2, 3, 0 };

	return flt_rounds[__libc_fenv_round(__libc_fenv_read())];
}
