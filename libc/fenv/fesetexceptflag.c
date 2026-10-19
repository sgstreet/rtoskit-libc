/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fesetexceptflag.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <fenv.h>

#include "fenv-arch.h"

/* C23 7.6.4.5 */
int fesetexceptflag(const fexcept_t *flagp, int excepts)
{
	unsigned int state;
	unsigned int flags;

	excepts &= FE_ALL_EXCEPT;
	state = __libc_fenv_read();
	flags = (__libc_fenv_flags(state) & ~(unsigned int)excepts) | (*flagp & (unsigned int)excepts);
	__libc_fenv_write(__libc_fenv_with_flags(state, flags));

	return 0;
}
