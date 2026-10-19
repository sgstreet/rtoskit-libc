/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * stdc-predef.h
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _STDC_PREDEF_H
#define _STDC_PREDEF_H

/*
 * The conditional feature macros of C23 6.10.10.3 that describe the library
 * rather than the compiler (PLAN D50). A hosted GCC includes this header
 * before every translation unit; with -nostdinc the toolchain files pass
 * -include for it. Annex F (IEC 60559 binary floating point) is claimed only
 * where <fenv.h> supports every exception and rounding direction (D44):
 * x86_64 and Arm with a double-precision FPU. Elsewhere double arithmetic is
 * libgcc software that neither rounds directionally nor raises flags. Annex G
 * (D49), decimal floating point, the interchange types and Annex K are not
 * claimed; the header is identical for every core of a target (D15).
 */
#if defined(__x86_64__) || (defined(__ARM_FP) && (__ARM_FP & 8))
#define __STDC_IEC_559__ 1
#define __STDC_IEC_60559_BFP__ 202311L
#endif

#endif /* _STDC_PREDEF_H */
