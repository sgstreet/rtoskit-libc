/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fenv-arch.h
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _FENV_ARCH_H
#define _FENV_ARCH_H

#include <fenv.h>

/*
 * Cores where double (or every) floating type is libgcc's software floating
 * point (D44): it rounds to nearest and records no exceptions, so there is no
 * state; only FE_TONEAREST exists and FE_ALL_EXCEPT is 0.
 */
#define __LIBC_FENV_DEFAULT 0u
#define __LIBC_FENV_FLAG_BITS 0u

static inline unsigned int __libc_fenv_read(void)
{
	return __LIBC_FENV_DEFAULT;
}

static inline void __libc_fenv_write(unsigned int)
{
}

static inline unsigned int __libc_fenv_flags(unsigned int)
{
	return 0;
}

static inline unsigned int __libc_fenv_with_flags(unsigned int state, unsigned int)
{
	return state;
}

static inline int __libc_fenv_round(unsigned int)
{
	return FE_TONEAREST;
}

static inline unsigned int __libc_fenv_with_round(unsigned int state, int)
{
	return state;
}

static inline bool __libc_fenv_round_valid(int round)
{
	return round == FE_TONEAREST;
}

static inline unsigned int __libc_fenv_nonstop(unsigned int state)
{
	return state;
}

/* Whether the library's exception-raising helpers need to do anything: no exception is recorded (PLAN D55) */
static constexpr bool __libc_fenv_records = false;

#endif /* _FENV_ARCH_H */
