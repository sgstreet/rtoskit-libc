/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * feholdexcept.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <fenv.h>

#include "fenv-arch.h"

/* C23 7.6.6.2: save, clear the flags, and stop on no exception */
int feholdexcept(fenv_t *envp)
{
	unsigned int state = __libc_fenv_read();

	envp->__state = state;
	__libc_fenv_write(__libc_fenv_nonstop(__libc_fenv_with_flags(state, 0)));

	return 0;
}
