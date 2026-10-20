/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * complex.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <fenv.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <tgmath.h>

/*
 * <complex.h> and <tgmath.h> (P1.16, docs/research/libm-complex.md, D49).
 * Reference vectors (600-bit mpmath, both parts rounded) bound the normwise
 * error: max(|re error|, |im error|) at most 4 ulp of the larger part for
 * double and 2 for float, to nearest. Then the Annex G special cases the
 * functions follow (signs of zero, infinities and NaNs), CMPLX, and the
 * type-generic macros' choice of function.
 */

static int failures;

static void fail(int line, const char *what)
{
	printf("fail: line %d: %s\n", line, what);
	++failures;
}

#define CHECK(condition) ((condition) ? (void)0 : fail(__LINE__, #condition))

/* The exception bits in canonical FE_* values, whether or not this core defines the macros (D44) */
#define INVALID 0x01u
#define DIVBYZERO 0x02u
#define OVERFLOW 0x04u
#define UNDERFLOW 0x08u
#define INEXACT 0x10u

/* The condition holds and raised exactly these exceptions (those the core has) */
#define EXPECT(condition, flags)                                         \
	do {                                                                 \
		bool holds;                                                      \
		unsigned int raised;                                             \
                                                                         \
		feclearexcept(FE_ALL_EXCEPT);                                    \
		holds = (condition);                                             \
		raised = (unsigned int)fetestexcept(FE_ALL_EXCEPT);              \
		if (!holds || raised != ((flags) & (unsigned int)FE_ALL_EXCEPT)) \
			fail(__LINE__, #condition " raising " #flags);               \
	} while (false)

/* The condition holds in the rounding direction (skipped where the core has only nearest) */
#define IN_DIRECTION(direction, condition, flags) \
	do {                                          \
		if (fesetround(direction) == 0) {         \
			EXPECT(condition, flags);             \
			fesetround(0);                        \
		}                                         \
	} while (false)

enum function {
	CACOS,
	CASIN,
	CATAN,
	CCOS,
	CSIN,
	CTAN,
	CACOSH,
	CASINH,
	CATANH,
	CCOSH,
	CSINH,
	CTANH,
	CEXP,
	CLOG,
	CABS,
	CPOW,
	CSQRT,
	CARG,
	CACOSF,
	CASINF,
	CATANF,
	CCOSF,
	CSINF,
	CTANF,
	CACOSHF,
	CASINHF,
	CATANHF,
	CCOSHF,
	CSINHF,
	CTANHF,
	CEXPF,
	CLOGF,
	CABSF,
	CPOWF,
	CSQRTF,
	CARGF,
};

static const char *const names[] = { "cacos", "casin", "catan", "ccos", "csin", "ctan", "cacosh", "casinh", "catanh", "ccosh", "csinh", "ctanh", "cexp", "clog", "cabs", "cpow", "csqrt", "carg" };

static const struct {
	enum function function;
	double zr;
	double zi;
	double wr;
	double wi;
	double rr;
	double ri;
} vectors[] = {
#include "vectors.inc"
};

static double complex call(enum function function, double complex z, double complex w)
{
	float complex zf = (float complex)z;
	float complex wf = (float complex)w;

	switch (function) {
		case CACOS:
			return cacos(z);
		case CASIN:
			return casin(z);
		case CATAN:
			return catan(z);
		case CCOS:
			return ccos(z);
		case CSIN:
			return csin(z);
		case CTAN:
			return ctan(z);
		case CACOSH:
			return cacosh(z);
		case CASINH:
			return casinh(z);
		case CATANH:
			return catanh(z);
		case CCOSH:
			return ccosh(z);
		case CSINH:
			return csinh(z);
		case CTANH:
			return ctanh(z);
		case CEXP:
			return cexp(z);
		case CLOG:
			return clog(z);
		case CABS:
			return cabs(z);
		case CPOW:
			return cpow(z, w);
		case CSQRT:
			return csqrt(z);
		case CARG:
			return carg(z);
		case CACOSF:
			return cacosf(zf);
		case CASINF:
			return casinf(zf);
		case CATANF:
			return catanf(zf);
		case CCOSF:
			return ccosf(zf);
		case CSINF:
			return csinf(zf);
		case CTANF:
			return ctanf(zf);
		case CACOSHF:
			return cacoshf(zf);
		case CASINHF:
			return casinhf(zf);
		case CATANHF:
			return catanhf(zf);
		case CCOSHF:
			return ccoshf(zf);
		case CSINHF:
			return csinhf(zf);
		case CTANHF:
			return ctanhf(zf);
		case CEXPF:
			return cexpf(zf);
		case CLOGF:
			return clogf(zf);
		case CABSF:
			return cabsf(zf);
		case CPOWF:
			return cpowf(zf, wf);
		case CSQRTF:
			return csqrtf(zf);
		case CARGF:
			return cargf(zf);
	}

	return NAN;
}

static void check_vectors(void)
{
	double worst[36] = { 0 };

	for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
		bool single = vectors[i].function >= CACOSF;
		double complex got = call(vectors[i].function, CMPLX(vectors[i].zr, vectors[i].zi), CMPLX(vectors[i].wr, vectors[i].wi));
		double scale = fmax(fabs(vectors[i].rr), fabs(vectors[i].ri));
		int exponent = ilogb(scale);
		double ulp;
		double error;

		if (single)
			ulp = ldexp(1.0, (exponent < FLT_MIN_EXP - 1 ? FLT_MIN_EXP - 1 : exponent) - FLT_MANT_DIG + 1);
		else
			ulp = ldexp(1.0, (exponent < DBL_MIN_EXP - 1 ? DBL_MIN_EXP - 1 : exponent) - DBL_MANT_DIG + 1);
		error = fmax(fabs(creal(got) - vectors[i].rr), fabs(cimag(got) - vectors[i].ri)) / ulp;
		if (!(error <= (single ? 2.0 : 4.0))) {
			char message[240];

			snprintf(message, sizeof(message), "%s%s(%a%+ai, %a%+ai) = %a%+ai, want %a%+ai: %g ulp", names[vectors[i].function % 18], single ? "f" : "", vectors[i].zr, vectors[i].zi, vectors[i].wr, vectors[i].wi, creal(got), cimag(got), vectors[i].rr, vectors[i].ri, error);
			fail(__LINE__, message);
		}
		if (error > worst[vectors[i].function])
			worst[vectors[i].function] = error;
	}

	for (int i = 0; i < 36; ++i)
		printf("%s%s: %.3f ulp\n", names[i % 18], i >= 18 ? "f" : "", worst[i]);
}

/* Both parts equal, including the sign of zero, and NaN matching NaN */
static bool same(double complex got, double re, double im)
{
	bool real_ok = isnan(re) ? isnan(creal(got)) : creal(got) == re && signbit(creal(got)) == signbit(re);
	bool imag_ok = isnan(im) ? isnan(cimag(got)) : cimag(got) == im && signbit(cimag(got)) == signbit(im);

	return real_ok && imag_ok;
}

static void check_basics(void)
{
	double complex z = CMPLX(INFINITY, NAN);

	/* 7.3.9.3: CMPLX keeps infinite and NaN parts; I is the imaginary unit */
	CHECK(isinf(creal(z)) && isnan(cimag(z)) && same(CMPLX(-0.0, -0.0), -0.0, -0.0));
	CHECK(same(I * I, -1.0, 0.0) && same(CMPLXF(1.5f, -2.0f), 1.5, -2.0) && same(CMPLXL(3.0L, 4.0L), 3.0, 4.0));
	CHECK(same(conj(CMPLX(1.0, 0.0)), 1.0, -0.0) && same(conj(CMPLX(-2.0, -3.0)), -2.0, 3.0) && same(conjf(CMPLXF(0.0f, NAN)), 0.0, NAN));
	CHECK(same(cproj(CMPLX(-INFINITY, -3.0)), INFINITY, -0.0) && same(cproj(CMPLX(NAN, INFINITY)), INFINITY, 0.0) && same(cproj(CMPLX(1.0, 2.0)), 1.0, 2.0));
	CHECK(creal(CMPLX(5.0, 6.0)) == 5.0 && cimag(CMPLX(5.0, 6.0)) == 6.0 && crealf(CMPLXF(7.0f, 8.0f)) == 7.0f && cimagl(CMPLXL(9.0L, 1.0L)) == 1.0L);
	CHECK(cabs(CMPLX(3.0, 4.0)) == 5.0 && cabs(CMPLX(NAN, INFINITY)) == INFINITY && carg(CMPLX(-1.0, 0.0)) == 0x1.921fb54442d18p+1 && carg(CMPLX(-1.0, -0.0)) == -0x1.921fb54442d18p+1);
}

/* Beyond |x| = 709 csinh and ccosh scale by e^|x|/2 in two halves: sinh x times cos y (sin y) keeps the signs of both (P2.05e3b) */
static void check_hyperbolic_halves(void)
{
	double scale = 0.5 * exp(355.0) * exp(355.0);
	double complex value = csinh(CMPLX(710.0, 2.0));

	CHECK(creal(value) < 0 && fabs(creal(value) / (scale * cos(2.0)) - 1.0) < 0x1p-40 && cimag(value) > 0);
	value = csinh(CMPLX(-710.0, 2.0));
	CHECK(creal(value) > 0 && fabs(creal(value) / (scale * cos(2.0)) + 1.0) < 0x1p-40 && cimag(value) > 0);
	value = ccosh(CMPLX(710.0, -2.0));
	CHECK(creal(value) < 0 && cimag(value) < 0 && fabs(cimag(value) / (scale * sin(-2.0)) - 1.0) < 0x1p-40);
	value = ccosh(CMPLX(-710.0, -2.0));
	CHECK(creal(value) < 0 && cimag(value) > 0 && fabs(cimag(value) / (scale * sin(-2.0)) + 1.0) < 0x1p-40);
}

static void check_annex_g(void)
{
	const double pi = 0x1.921fb54442d18p+1;
	const double half_pi = 0x1.921fb54442d18p+0;

	/* G.6.1.1 cacos */
	CHECK(same(cacos(CMPLX(0.0, 0.0)), half_pi, -0.0) && same(cacos(CMPLX(-0.0, NAN)), half_pi, NAN));
	CHECK(same(cacos(CMPLX(2.0, INFINITY)), half_pi, -INFINITY) && same(cacos(CMPLX(-INFINITY, 1.0)), pi, -INFINITY));
	CHECK(same(cacos(CMPLX(INFINITY, 1.0)), 0.0, -INFINITY) && same(cacos(CMPLX(-INFINITY, INFINITY)), 0x1.2d97c7f3321d2p+1, -INFINITY));
	CHECK(same(cacos(CMPLX(INFINITY, INFINITY)), 0x1.921fb54442d18p-1, -INFINITY) && same(cacos(CMPLX(NAN, INFINITY)), NAN, -INFINITY));
	CHECK(isinf(cimag(cacos(CMPLX(INFINITY, NAN)))) && isnan(creal(cacos(CMPLX(INFINITY, NAN)))) && same(cacos(CMPLX(NAN, 1.0)), NAN, NAN));
	/* G.6.2.1 cacosh */
	CHECK(same(cacosh(CMPLX(0.0, 0.0)), 0.0, half_pi) && same(cacosh(CMPLX(1.0, INFINITY)), INFINITY, half_pi));
	CHECK(same(cacosh(CMPLX(-INFINITY, 1.0)), INFINITY, pi) && same(cacosh(CMPLX(INFINITY, 1.0)), INFINITY, 0.0));
	CHECK(same(cacosh(CMPLX(INFINITY, NAN)), INFINITY, NAN) && same(cacosh(CMPLX(NAN, INFINITY)), INFINITY, NAN) && same(cacosh(CMPLX(1.0, -0.0)), 0.0, -0.0));
	/* G.6.2.2 casinh */
	CHECK(same(casinh(CMPLX(0.0, 0.0)), 0.0, 0.0) && same(casinh(CMPLX(-0.0, -0.0)), -0.0, -0.0));
	CHECK(same(casinh(CMPLX(1.0, INFINITY)), INFINITY, half_pi) && same(casinh(CMPLX(INFINITY, 1.0)), INFINITY, 0.0));
	CHECK(same(casinh(CMPLX(INFINITY, INFINITY)), INFINITY, 0x1.921fb54442d18p-1) && same(casinh(CMPLX(INFINITY, NAN)), INFINITY, NAN));
	CHECK(same(casinh(CMPLX(NAN, 0.0)), NAN, 0.0) && isinf(creal(casinh(CMPLX(NAN, INFINITY)))) && same(casinh(CMPLX(2.0, NAN)), NAN, NAN));
	/* G.6.2.3 catanh */
	CHECK(same(catanh(CMPLX(0.0, 0.0)), 0.0, 0.0) && same(catanh(CMPLX(0.0, NAN)), 0.0, NAN));
	CHECK(same(catanh(CMPLX(1.0, 0.0)), INFINITY, 0.0) && same(catanh(CMPLX(-1.0, -0.0)), -INFINITY, -0.0));
	CHECK(same(catanh(CMPLX(2.0, INFINITY)), 0.0, half_pi) && same(catanh(CMPLX(INFINITY, 2.0)), 0.0, half_pi) && same(catanh(CMPLX(-INFINITY, -2.0)), -0.0, -half_pi));
	CHECK(same(catanh(CMPLX(INFINITY, NAN)), 0.0, NAN) && same(catanh(CMPLX(NAN, INFINITY)), 0.0, half_pi) && same(catanh(CMPLX(2.0, NAN)), NAN, NAN));
	/* G.6.2.4 ccosh */
	CHECK(same(ccosh(CMPLX(0.0, 0.0)), 1.0, 0.0) && same(ccosh(CMPLX(-0.0, 0.0)), 1.0, -0.0));
	CHECK(isnan(creal(ccosh(CMPLX(0.0, INFINITY)))) && cimag(ccosh(CMPLX(0.0, INFINITY))) == 0 && same(ccosh(CMPLX(INFINITY, 0.0)), INFINITY, 0.0));
	CHECK(same(ccosh(CMPLX(INFINITY, 1.0)), INFINITY, INFINITY) && same(ccosh(CMPLX(-INFINITY, 1.0)), INFINITY, -INFINITY) && same(ccosh(CMPLX(INFINITY, NAN)), INFINITY, NAN));
	CHECK(same(ccosh(CMPLX(2.0, INFINITY)), NAN, NAN) && same(ccosh(CMPLX(NAN, 1.0)), NAN, NAN) && isnan(creal(ccosh(CMPLX(NAN, 0.0)))) && cimag(ccosh(CMPLX(NAN, 0.0))) == 0);
	/* G.6.2.5 csinh */
	CHECK(same(csinh(CMPLX(0.0, 0.0)), 0.0, 0.0) && same(csinh(CMPLX(-0.0, -0.0)), -0.0, -0.0));
	CHECK(creal(csinh(CMPLX(0.0, INFINITY))) == 0 && isnan(cimag(csinh(CMPLX(0.0, INFINITY)))) && same(csinh(CMPLX(INFINITY, 0.0)), INFINITY, 0.0));
	CHECK(same(csinh(CMPLX(INFINITY, 1.0)), INFINITY, INFINITY) && same(csinh(CMPLX(-INFINITY, 1.0)), -INFINITY, INFINITY) && same(csinh(CMPLX(NAN, 0.0)), NAN, 0.0));
	CHECK(isinf(creal(csinh(CMPLX(INFINITY, INFINITY)))) && isnan(cimag(csinh(CMPLX(INFINITY, INFINITY)))) && same(csinh(CMPLX(2.0, NAN)), NAN, NAN));
	/* G.6.2.6 ctanh */
	CHECK(same(ctanh(CMPLX(0.0, 0.0)), 0.0, 0.0) && same(ctanh(CMPLX(-0.0, -0.0)), -0.0, -0.0) && same(ctanh(CMPLX(0.0, INFINITY)), 0.0, NAN));
	CHECK(same(ctanh(CMPLX(2.0, INFINITY)), NAN, NAN) && same(ctanh(CMPLX(INFINITY, 1.0)), 1.0, 0.0) && same(ctanh(CMPLX(INFINITY, INFINITY)), 1.0, 0.0));
	CHECK(same(ctanh(CMPLX(-INFINITY, NAN)), -1.0, 0.0) && same(ctanh(CMPLX(NAN, 0.0)), NAN, 0.0) && same(ctanh(CMPLX(NAN, 1.0)), NAN, NAN));
	CHECK(same(ctanh(CMPLX(400.0, 1.0)), 1.0, 0.0) && same(ctanh(CMPLX(-400.0, -1.0)), -1.0, -0.0));
	/* G.6.3.1 cexp, G.6.3.2 clog */
	CHECK(same(cexp(CMPLX(0.0, 0.0)), 1.0, 0.0) && same(cexp(CMPLX(-0.0, -0.0)), 1.0, -0.0) && same(cexp(CMPLX(INFINITY, 0.0)), INFINITY, 0.0));
	CHECK(same(cexp(CMPLX(-INFINITY, 1.0)), 0.0, 0.0) && same(cexp(CMPLX(INFINITY, -1.0)), INFINITY, -INFINITY) && same(cexp(CMPLX(NAN, 0.0)), NAN, 0.0));
	CHECK(creal(cexp(CMPLX(-INFINITY, INFINITY))) == 0 && cimag(cexp(CMPLX(-INFINITY, NAN))) == 0 && same(cexp(CMPLX(2.0, INFINITY)), NAN, NAN));
	CHECK(isinf(creal(cexp(CMPLX(INFINITY, NAN)))) && isnan(cimag(cexp(CMPLX(INFINITY, NAN)))) && isfinite(creal(cexp(CMPLX(709.9, 1.5)))));
	CHECK(same(clog(CMPLX(-0.0, 0.0)), -INFINITY, pi) && same(clog(CMPLX(0.0, -0.0)), -INFINITY, -0.0) && same(clog(CMPLX(-INFINITY, 1.0)), INFINITY, pi));
	CHECK(same(clog(CMPLX(INFINITY, -1.0)), INFINITY, -0.0) && same(clog(CMPLX(NAN, INFINITY)), INFINITY, NAN) && same(clog(CMPLX(1.0, NAN)), NAN, NAN));
	/* Near the unit circle the real part keeps its own relative accuracy (the normwise vector bound would hide it) */
	CHECK(creal(clog(CMPLX(0.6, 0.8))) == 0x1.999999999999ap-56 && creal(clog(CMPLX(1 + 0x1p-40, 0x1p-21))) == 0x1.1fffffffff3c0p-40);
	CHECK(same(clog(CMPLX(1.0, 0.0)), 0.0, 0.0) && fabs(creal(clog(CMPLX(DBL_MAX, DBL_MAX))) - 0x1.63108c75a1936p+9) <= 0x1p-43);
	/* G.6.4.2 csqrt */
	CHECK(same(csqrt(CMPLX(-0.0, 0.0)), 0.0, 0.0) && same(csqrt(CMPLX(0.0, -0.0)), 0.0, -0.0) && same(csqrt(CMPLX(NAN, INFINITY)), INFINITY, INFINITY));
	CHECK(same(csqrt(CMPLX(-INFINITY, 1.0)), 0.0, INFINITY) && same(csqrt(CMPLX(INFINITY, -1.0)), INFINITY, -0.0) && same(csqrt(CMPLX(INFINITY, NAN)), INFINITY, NAN));
	CHECK(isnan(creal(csqrt(CMPLX(-INFINITY, NAN)))) && isinf(cimag(csqrt(CMPLX(-INFINITY, NAN)))) && same(csqrt(CMPLX(2.0, NAN)), NAN, NAN));
	CHECK(same(csqrt(CMPLX(-4.0, 0.0)), 0.0, 2.0) && same(csqrt(CMPLX(-4.0, -0.0)), 0.0, -2.0) && fabs(creal(csqrt(CMPLX(DBL_MAX, DBL_MAX))) - 0x1.19435caffa9f8p+512) <= 0x1p461 && fabs(cimag(csqrt(CMPLX(DBL_MAX, DBL_MAX))) - 0x1.d203138f6c828p+510) <= 0x1p459);
	/* The trigonometric functions through the rotations (G.6) and cpow's simple cases */
	CHECK(same(csin(CMPLX(0.0, 0.0)), 0.0, 0.0) && same(ccos(CMPLX(0.0, 0.0)), 1.0, -0.0) && same(ctan(CMPLX(-0.0, 0.0)), -0.0, 0.0));
	CHECK(same(casin(CMPLX(0.0, -0.0)), 0.0, -0.0) && same(catan(CMPLX(0.0, 1.0)), 0.0, INFINITY));
	CHECK(same(cpow(CMPLX(2.0, 3.0), CMPLX(0.0, 0.0)), 1.0, 0.0) && same(cpow(CMPLX(0.0, 0.0), CMPLX(2.0, 0.0)), 0.0, 0.0) && same(cpow(CMPLX(2.0, 0.0), CMPLX(10.0, 0.0)), 1024.0, 0.0));
	CHECK(same(cexpf(CMPLXF(0.0f, 0.0f)), 1.0, 0.0) && same(csqrtl(CMPLXL(-9.0L, 0.0L)), 0.0, 3.0) && same(ccoshf(CMPLXF(INFINITY, 0.0f)), INFINITY, 0.0));
}

/* C23 7.27: the type of a type-generic macro's result names the function it chose */
#define TYPE(expression) _Generic((expression), float: 1, double: 2, long double: 3, float complex: 4, double complex: 5, long double complex: 6, int: 7, long: 8, long long: 9, default: 0)

static void check_tgmath(void)
{
	float f = 2.0f;
	double d = 2.0;
	long double l = 2.0L;
	float complex fc = CMPLXF(1.0f, 1.0f);
	double complex dc = CMPLX(1.0, 1.0);
	long double complex lc = CMPLXL(1.0L, 1.0L);
	int n = 2;
	int q;

	CHECK(TYPE(sin(f)) == 1 && TYPE(sin(d)) == 2 && TYPE(sin(l)) == 3 && TYPE(sin(n)) == 2);
	CHECK(TYPE(sin(fc)) == 4 && TYPE(sin(dc)) == 5 && TYPE(sin(lc)) == 6 && TYPE(fabs(fc)) == 1 && TYPE(fabs(lc)) == 3);
	CHECK(TYPE(pow(f, f)) == 1 && TYPE(pow(f, n)) == 2 && TYPE(pow(f, l)) == 3 && TYPE(pow(f, fc)) == 4 && TYPE(pow(d, fc)) == 5);
	CHECK(TYPE(atan2(f, f)) == 1 && TYPE(atan2(f, d)) == 2 && TYPE(fma(f, f, l)) == 3 && TYPE(fma(f, f, f)) == 1 && TYPE(fma(f, n, f)) == 2);
	CHECK(TYPE(ldexp(f, 3)) == 1 && TYPE(frexp(l, &q)) == 3 && TYPE(remquo(f, f, &q)) == 1 && TYPE(nexttoward(f, l)) == 1 && TYPE(scalbn(n, 2)) == 2);
	CHECK(TYPE(ilogb(f)) == 7 && TYPE(lround(l)) == 8 && TYPE(llrint(f)) == 9 && TYPE(fromfp(f, FP_INT_UPWARD, 8)) == 1 && TYPE(pown(f, 3)) == 1 && TYPE(rootn(l, 3)) == 3);
	CHECK(TYPE(creal(f)) == 1 && TYPE(cimag(dc)) == 2 && TYPE(carg(lc)) == 3 && TYPE(conj(f)) == 4 && TYPE(cproj(n)) == 5 && TYPE(conj(lc)) == 6);
	CHECK(TYPE(fadd(d, d)) == 1 && TYPE(fadd(d, l)) == 1 && TYPE(dadd(l, l)) == 2 && TYPE(ffma(d, d, d)) == 1 && TYPE(dsqrt(l)) == 2 && TYPE(fsqrt(n)) == 1);
	CHECK(sqrt(f) == sqrtf(2.0f) && sqrt(n) == 0x1.6a09e667f3bcdp+0 && creal(sqrt(CMPLX(-4.0, 0.0))) == 0 && cimag(sqrt(CMPLX(-4.0, 0.0))) == 2.0);
	CHECK(exp(0) == 1.0 && cimag(exp(CMPLXF(0.0f, 0.0f))) == 0 && cbrt(27) == 3.0 && fmax(n, 3.5f) == 3.5 && cospi(n) == 1.0);
	CHECK(__STDC_VERSION_TGMATH_H__ == 202311L && __STDC_VERSION_COMPLEX_H__ == 202311L);
}

int main(void)
{
	check_vectors();
	check_basics();
	check_hyperbolic_halves();
	check_annex_g();
	check_tgmath();

	if (failures == 0)
		printf("pass: complex (%zu vectors)\n", sizeof(vectors) / sizeof(vectors[0]));

	return failures == 0 ? 0 : 1;
}
