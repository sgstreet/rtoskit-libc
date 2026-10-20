/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * aeabi.h
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_AEABI_H
#define _INTERNAL_AEABI_H

/*
 * The soft-float helpers of the Arm run-time ABI (RTABI 4.1.2), which the
 * compiler calls for floating-point arithmetic on cores without an FPU. The
 * compiler's library provides them; the nofp math layer replaces the two
 * subtractions (PLAN D55).
 */
double __aeabi_dadd(double a, double b);
double __aeabi_dsub(double a, double b);
float __aeabi_fadd(float a, float b);
float __aeabi_fsub(float a, float b);

#endif /* _INTERNAL_AEABI_H */
