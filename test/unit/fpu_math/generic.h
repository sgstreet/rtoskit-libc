/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * generic.h
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _GENERIC_H
#define _GENERIC_H

/* The generic C versions of the functions FPU layers replace, compiled into the test (generic.c) */
double generic_fabs(double x);
float generic_fabsf(float x);
double generic_fma(double x, double y, double z);
float generic_fmaf(float x, float y, float z);
double generic_rint(double x);
float generic_rintf(float x);
double generic_nearbyint(double x);
float generic_nearbyintf(float x);
double generic_trunc(double x);
float generic_truncf(float x);
double generic_floor(double x);
float generic_floorf(float x);
double generic_ceil(double x);
float generic_ceilf(float x);
double generic_round(double x);
float generic_roundf(float x);
double generic_roundeven(double x);
float generic_roundevenf(float x);
double generic_fmax(double x, double y);
float generic_fmaxf(float x, float y);
double generic_fmin(double x, double y);
float generic_fminf(float x, float y);

#endif /* _GENERIC_H */
