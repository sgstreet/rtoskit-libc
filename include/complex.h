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

#ifndef _COMPLEX_H
#define _COMPLEX_H

/*
 * C23 7.3 (docs/research/libm-complex.md). Imaginary types are not
 * supported (no imaginary or _Imaginary_I), and __STDC_IEC_60559_COMPLEX__ is
 * not claimed (D49); the functions follow the Annex G special cases anyway.
 */

#define __STDC_VERSION_COMPLEX_H__ 202311L

#define complex _Complex
#define _Complex_I (__builtin_complex(0.0f, 1.0f))
#define I _Complex_I

/* C23 7.3.9.3: the value x + iy without the arithmetic that would lose an infinite or NaN part */
#define CMPLX(x, y) __builtin_complex((double)(x), (double)(y))
#define CMPLXF(x, y) __builtin_complex((float)(x), (float)(y))
#define CMPLXL(x, y) __builtin_complex((long double)(x), (long double)(y))

#ifdef __cplusplus
extern "C" {
#endif

double complex cacos(double complex z);
double complex casin(double complex z);
double complex catan(double complex z);
double complex ccos(double complex z);
double complex csin(double complex z);
double complex ctan(double complex z);
double complex cacosh(double complex z);
double complex casinh(double complex z);
double complex catanh(double complex z);
double complex ccosh(double complex z);
double complex csinh(double complex z);
double complex ctanh(double complex z);
double complex cexp(double complex z);
double complex clog(double complex z);
double cabs(double complex z);
double complex cpow(double complex z, double complex w);
double complex csqrt(double complex z);
double carg(double complex z);
double cimag(double complex z);
double complex conj(double complex z);
double complex cproj(double complex z);
double creal(double complex z);

float complex cacosf(float complex z);
float complex casinf(float complex z);
float complex catanf(float complex z);
float complex ccosf(float complex z);
float complex csinf(float complex z);
float complex ctanf(float complex z);
float complex cacoshf(float complex z);
float complex casinhf(float complex z);
float complex catanhf(float complex z);
float complex ccoshf(float complex z);
float complex csinhf(float complex z);
float complex ctanhf(float complex z);
float complex cexpf(float complex z);
float complex clogf(float complex z);
float cabsf(float complex z);
float complex cpowf(float complex z, float complex w);
float complex csqrtf(float complex z);
float cargf(float complex z);
float cimagf(float complex z);
float complex conjf(float complex z);
float complex cprojf(float complex z);
float crealf(float complex z);

long double complex cacosl(long double complex z);
long double complex casinl(long double complex z);
long double complex catanl(long double complex z);
long double complex ccosl(long double complex z);
long double complex csinl(long double complex z);
long double complex ctanl(long double complex z);
long double complex cacoshl(long double complex z);
long double complex casinhl(long double complex z);
long double complex catanhl(long double complex z);
long double complex ccoshl(long double complex z);
long double complex csinhl(long double complex z);
long double complex ctanhl(long double complex z);
long double complex cexpl(long double complex z);
long double complex clogl(long double complex z);
long double cabsl(long double complex z);
long double complex cpowl(long double complex z, long double complex w);
long double complex csqrtl(long double complex z);
long double cargl(long double complex z);
long double cimagl(long double complex z);
long double complex conjl(long double complex z);
long double complex cprojl(long double complex z);
long double creall(long double complex z);

#ifdef __cplusplus
}
#endif

#endif /* _COMPLEX_H */
