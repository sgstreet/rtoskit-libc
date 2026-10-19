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
 * The FPSCR behind <fenv.h> on an Arm core whose FPU implements every
 * floating type (D44). The FE_* values are its cumulative flag bits (IOC,
 * DZC, OFC, UFC, IXC; IDC 0x80 is no C exception) and its RMode encoding in
 * bits 22-23. Trap enables (bits 8-12, 15) do not exist on M-profile cores.
 */
#define __LIBC_FENV_DEFAULT 0u
#define __LIBC_FENV_FLAG_BITS 0x9fu

static inline unsigned int __libc_fenv_read(void)
{
	unsigned int state;

	__asm__ volatile("vmrs %0, fpscr" : "=r"(state));
	return state;
}

static inline void __libc_fenv_write(unsigned int state)
{
	__asm__ volatile("vmsr fpscr, %0" ::"r"(state));
}

static inline unsigned int __libc_fenv_flags(unsigned int state)
{
	return state & FE_ALL_EXCEPT;
}

static inline unsigned int __libc_fenv_with_flags(unsigned int state, unsigned int flags)
{
	return (state & ~(unsigned int)FE_ALL_EXCEPT) | (flags & FE_ALL_EXCEPT);
}

static inline int __libc_fenv_round(unsigned int state)
{
	return (int)((state >> 22) & 3);
}

static inline unsigned int __libc_fenv_with_round(unsigned int state, int round)
{
	return (state & ~(3u << 22)) | ((unsigned int)round << 22);
}

static inline bool __libc_fenv_round_valid(int round)
{
	return round >= FE_TONEAREST && round <= FE_TOWARDZERO;
}

static inline unsigned int __libc_fenv_nonstop(unsigned int state)
{
	return state & ~0x9f00u;
}

#endif /* _FENV_ARCH_H */
