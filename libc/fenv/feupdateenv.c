/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * feupdateenv.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <fenv.h>

#include "fenv-arch.h"

/* C23 7.6.6.4: install the environment, then raise the exceptions that were raised */
int feupdateenv(const fenv_t *envp)
{
	unsigned int raised = __libc_fenv_flags(__libc_fenv_read());

	fesetenv(envp);
	return feraiseexcept((int)raised);
}
