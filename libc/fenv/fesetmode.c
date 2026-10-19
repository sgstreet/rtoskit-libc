/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fesetmode.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <fenv.h>

#include "fenv-arch.h"

/* C23 7.6.5.4: the flags stay */
int fesetmode(const femode_t *modep)
{
	unsigned int mode = modep == FE_DFL_MODE ? __LIBC_FENV_DEFAULT : modep->__mode;

	__libc_fenv_write((__libc_fenv_read() & __LIBC_FENV_FLAG_BITS) | (mode & ~__LIBC_FENV_FLAG_BITS));

	return 0;
}
