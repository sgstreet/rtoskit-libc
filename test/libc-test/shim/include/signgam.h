/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * signgam.h
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _SIGNGAM_H
#define _SIGNGAM_H

#include <math.h>

/*
 * The XSI signgam that libc-test's lgamma tests read (POSIX, outside the ISO C
 * scope of the library, PLAN §1; C does not reserve the name, so libc cannot
 * define it). Forced into the math suite with -include: the wrappers record
 * the sign of gamma(x) and return the library's lgamma unchanged.
 */
extern int signgam;

double __shim_lgamma(double x);
float __shim_lgammaf(float x);
long double __shim_lgammal(long double x);

#define lgamma(x) __shim_lgamma(x)
#define lgammaf(x) __shim_lgammaf(x)
#define lgammal(x) __shim_lgammal(x)

#endif /* _SIGNGAM_H */
