/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fegetexceptflag.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <fenv.h>

#include "fenv-arch.h"

/* C23 7.6.4.2 */
int fegetexceptflag(fexcept_t *flagp, int excepts)
{
	excepts &= FE_ALL_EXCEPT;
	*flagp = __libc_fenv_flags(__libc_fenv_read()) & (unsigned int)excepts;

	return 0;
}
