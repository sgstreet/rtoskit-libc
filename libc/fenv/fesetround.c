/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fesetround.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <fenv.h>

#include "fenv-arch.h"

/* C23 7.6.5.5: only the directions the core supports (D44) */
int fesetround(int rnd)
{
	if (!__libc_fenv_round_valid(rnd))
		return 1;
	__libc_fenv_write(__libc_fenv_with_round(__libc_fenv_read(), rnd));

	return 0;
}
