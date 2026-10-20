/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * complex.h
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_COMPLEX_H
#define _INTERNAL_COMPLEX_H

#include <complex.h>

/*
 * The parts of a complex value through a union (C23 6.2.5: a complex type
 * has the representation of an array of two reals), and the rotations by i
 * the functions are related by (csin(z) = -i csinh(iz), ...).
 */
union __libc_complex {
	double complex z;
	double parts[2];
};

static inline double __libc_re(double complex z)
{
	return ((union __libc_complex){ .z = z }).parts[0];
}

static inline double __libc_im(double complex z)
{
	return ((union __libc_complex){ .z = z }).parts[1];
}

union __libc_complexf {
	float complex z;
	float parts[2];
};

static inline float __libc_ref(float complex z)
{
	return ((union __libc_complexf){ .z = z }).parts[0];
}

static inline float __libc_imf(float complex z)
{
	return ((union __libc_complexf){ .z = z }).parts[1];
}

/* i z and -i z, exactly (no arithmetic on the parts) */
static inline double complex __libc_times_i(double complex z)
{
	return CMPLX(-__libc_im(z), __libc_re(z));
}

static inline double complex __libc_times_minus_i(double complex z)
{
	return CMPLX(__libc_im(z), -__libc_re(z));
}

#endif /* _INTERNAL_COMPLEX_H */
