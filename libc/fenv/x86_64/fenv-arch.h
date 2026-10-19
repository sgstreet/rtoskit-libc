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
 * The SSE control and status register (MXCSR) behind <fenv.h> on x86_64
 * (D44). Flags: IE 0x01, DE 0x02 (no C exception), ZE 0x04, OE 0x08,
 * UE 0x10, PE 0x20; masks 0x1f80; rounding control in bits 13-14 (nearest,
 * down, up, toward zero).
 */
#define __LIBC_FENV_DEFAULT 0x1f80u
#define __LIBC_FENV_FLAG_BITS 0x3fu

static inline unsigned int __libc_fenv_read(void)
{
	unsigned int state;

	__asm__ volatile("stmxcsr %0" : "=m"(state));
	return state;
}

static inline void __libc_fenv_write(unsigned int state)
{
	__asm__ volatile("ldmxcsr %0" ::"m"(state));
}

/* FE_INVALID is IE; the others are one bit lower than in MXCSR */
static inline unsigned int __libc_fenv_flags(unsigned int state)
{
	return (state & 0x01u) | ((state >> 1) & 0x1eu);
}

static inline unsigned int __libc_fenv_with_flags(unsigned int state, unsigned int flags)
{
	return (state & ~0x3du) | (flags & 0x01u) | ((flags & 0x1eu) << 1);
}

/* MXCSR orders the directions nearest, down, up, zero; FE_* nearest, up, down, zero */
static inline int __libc_fenv_round(unsigned int state)
{
	static const int directions[4] = { FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO };

	return directions[(state >> 13) & 3];
}

static inline unsigned int __libc_fenv_with_round(unsigned int state, int round)
{
	static const unsigned int control[4] = { 0, 2, 1, 3 };

	return (state & ~(3u << 13)) | (control[round & 3] << 13);
}

static inline bool __libc_fenv_round_valid(int round)
{
	return round >= FE_TONEAREST && round <= FE_TOWARDZERO;
}

/* Non-stop mode: every exception masked */
static inline unsigned int __libc_fenv_nonstop(unsigned int state)
{
	return state | 0x1f80u;
}

#endif /* _FENV_ARCH_H */
