/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * libc-fpu.h
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _LIBC_FPU_H
#define _LIBC_FPU_H

/*
 * What the FPU computes in one instruction, for the private libm headers
 * (PLAN D82; libc/internal/fpu/${LIBC_FPU} is on CPPFLAGS): Armv6-M: no FPU, every float operation is a call.
 */

/* fma is one instruction, so a double product's exact error is too */
constexpr bool __libc_fpu_fma = false;

/* fmaf is one instruction, so a float product's exact error is too */
constexpr bool __libc_fpu_fmaf = false;

#endif /* _LIBC_FPU_H */
