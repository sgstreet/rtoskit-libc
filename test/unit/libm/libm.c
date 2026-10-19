/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * libm.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#define __STDC_WANT_IEC_60559_EXT__

#include <fenv.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * libm part 1 (P1.13, docs/research/fenv-libm.md): exact vectors for fma
 * and the narrowing functions in every rounding direction the core supports
 * (FE_* values 0-3: nearest, upward, downward, toward zero), the C23
 * functions libc-test does not cover, and <fenv.h>'s C23 additions.
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
#define UNDERFLOW 0x08u
#define INEXACT 0x10u

enum function {
	FMA,
	FMAF,
	FFMA,
	FADD,
	FSUB,
	FMUL,
	FDIV,
	FSQRT,
	DSQRTL,
};

static const char *const names[] = { "fma", "fmaf", "ffma", "fadd", "fsub", "fmul", "fdiv", "fsqrt", "dsqrtl" };

static const struct {
	enum function function;
	int direction;
	uint64_t a;
	uint64_t b;
	uint64_t c;
	uint64_t result;
	unsigned int flags;
} vectors[] = {
#include "vectors.inc"
};

static double d(uint64_t bits)
{
	double x;

	memcpy(&x, &bits, sizeof(x));
	return x;
}

static float f(uint64_t bits)
{
	uint32_t low = (uint32_t)bits;
	float x;

	memcpy(&x, &low, sizeof(x));
	return x;
}

static uint64_t bits_d(double x)
{
	uint64_t bits;

	memcpy(&bits, &x, sizeof(bits));
	return bits;
}

static uint64_t bits_f(float x)
{
	uint32_t bits;

	memcpy(&bits, &x, sizeof(bits));
	return bits;
}

static void check_vectors(void)
{
	int run = 0;

	for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
		uint64_t result = 0;
		unsigned int raised;
		unsigned int mask = FE_ALL_EXCEPT;
		bool is_float = vectors[i].function != FMA && vectors[i].function != DSQRTL;

		if (fesetround(vectors[i].direction) != 0)
			continue;
		++run;
		feclearexcept(FE_ALL_EXCEPT);
		switch (vectors[i].function) {
			case FMA:
				result = bits_d(fma(d(vectors[i].a), d(vectors[i].b), d(vectors[i].c)));
				break;
			case FMAF:
				result = bits_f(fmaf(f(vectors[i].a), f(vectors[i].b), f(vectors[i].c)));
				break;
			case FFMA:
				result = bits_f(ffma(d(vectors[i].a), d(vectors[i].b), d(vectors[i].c)));
				break;
			case FADD:
				result = bits_f(fadd(d(vectors[i].a), d(vectors[i].b)));
				break;
			case FSUB:
				result = bits_f(fsub(d(vectors[i].a), d(vectors[i].b)));
				break;
			case FMUL:
				result = bits_f(fmul(d(vectors[i].a), d(vectors[i].b)));
				break;
			case FDIV:
				result = bits_f(fdiv(d(vectors[i].a), d(vectors[i].b)));
				break;
			case FSQRT:
				result = bits_f(fsqrt(d(vectors[i].a)));
				break;
			case DSQRTL:
				result = bits_d(dsqrtl(d(vectors[i].a)));
				break;
		}
		raised = (unsigned int)fetestexcept(FE_ALL_EXCEPT);
		fesetround(FE_TONEAREST);

		/* A result rounded up to the smallest normal is tiny before rounding only (Arm), not after (x86) */
		if ((is_float && (result & 0x7fffffff) == 0x00800000) || (!is_float && (result & 0x7fffffffffffffff) == 0x0010000000000000))
			mask &= ~UNDERFLOW;
		if (result != vectors[i].result || (raised & mask) != (vectors[i].flags & mask)) {
			char message[160];

			snprintf(message, sizeof(message), "%s(%016llx, %016llx, %016llx) direction %d = %016llx flags %02x, want %016llx flags %02x", names[vectors[i].function], (unsigned long long)vectors[i].a, (unsigned long long)vectors[i].b, (unsigned long long)vectors[i].c, vectors[i].direction, (unsigned long long)result, raised & mask, (unsigned long long)vectors[i].result, vectors[i].flags & mask);
			fail(__LINE__, message);
		}
	}
	CHECK(run >= (int)(sizeof(vectors) / sizeof(vectors[0]) / 4));
}

/* Special operands, which the vectors leave out */
static void check_special(void)
{
	double inf = INFINITY;
	double nan = NAN;

	feclearexcept(FE_ALL_EXCEPT);
	CHECK(isnan(fma(inf, 0.0, 1.0)) && (fetestexcept(FE_ALL_EXCEPT) & INVALID) == (FE_ALL_EXCEPT & INVALID));
	CHECK(isnan(fma(inf, 1.0, -inf)) && isnan(fma(nan, 1.0, 1.0)) && isnan(fma(1.0, 1.0, nan)));
	CHECK(fma(1e300, 1e300, -inf) == -inf && fma(2.0, 3.0, inf) == inf && fma(inf, 2.0, 1.0) == inf);
	CHECK(fma(0.0, 5.0, -0.0) == 0 && !signbit(fma(0.0, 5.0, -0.0)) && signbit(fma(-0.0, 5.0, -0.0)));
	/* An exact zero sum is +0 to nearest, and a nonzero product with a zero addend keeps its sign */
	CHECK(!signbit(fma(2.0, 3.0, -6.0)) && signbit(fma(-0x1p-600, 0x1p-600, 0.0)));
	CHECK(fma(0x1p-600, 0x1p-600, 0.0) == 0 && fmaf(0x1p-100f, 0x1p-100f, 0.0f) == 0 && signbit(fmaf(-0x1p-100f, 0x1p-100f, 0.0f)));
	CHECK(fadd(1.0, -1.0) == 0 && !signbit(fadd(1.0, -1.0)) && signbit(fadd(-0.0, -0.0)) && fadd(inf, 1.0) == INFINITY && isnan(fadd(inf, -inf)));
	CHECK(fsub(1.0, 1.0) == 0 && fmul(-0.0, 3.0) == 0 && signbit(fmul(-0.0, 3.0)) && fdiv(1.0, 0.0) == INFINITY && isnan(fdiv(0.0, 0.0)));
	CHECK(fdiv(1e300, 1e-300) == INFINITY && fmul(1e200, 1e200) == INFINITY && fadd(0x1p128, 0.0) == INFINITY);
	CHECK(isnan(fsqrt(-1.0)) && fsqrt(-0.0) == 0 && signbit(fsqrt(-0.0)) && fsqrt(inf) == INFINITY && fsqrt(4.0) == 2.0f);
	CHECK(isnan(dsqrtl(-1.0)) && dsqrtl(2.0) == 0x1.6a09e667f3bcdp+0 && dsqrtl(0x1p-1074) == 0x1p-537);
	/* Ties at the subnormal precision round to even (the software path rounds those itself) */
	CHECK(fma(1.5, 0x1p-1074, 0x1p-1074) == 0x1p-1073 && fma(2.5, 0x1p-1074, 0x1p-1074) == 0x1p-1072 && fmaf(1.5f, 0x1p-149f, 0x1p-149f) == 0x1p-148f && fmaf(2.5f, 0x1p-149f, 0x1p-149f) == 0x1p-147f);
	/* Equal significands divide to an exact power of two, in every direction */
	if (fesetround(2) == 0) {
		CHECK(fdiv(3.0, 3.0) == 1.0f && fdiv(6.0, 3.0) == 2.0f && fdiv(-3.0, 0.75) == -4.0f);
		fesetround(FE_TONEAREST);
	}
	CHECK(daddl(1.0, 0x1p-60) == 1.0 && dsubl(1.0, 0.5) == 0.5 && dmull(3.0, 0.5) == 1.5 && ddivl(1.0, 4.0) == 0.25 && dfmal(2.0, 3.0, 1.0) == 7.0);
	CHECK(faddl(1.0L, 2.0L) == 3.0f && fsubl(1.0L, 2.0L) == -1.0f && fmull(2.0L, 2.0L) == 4.0f && fdivl(1.0L, 2.0L) == 0.5f && ffmal(2.0L, 2.0L, 1.0L) == 5.0f && fsqrtl(9.0L) == 3.0f);
}

/* C23 functions beyond libc-test */
static void check_c23(void)
{
	double qnan = NAN;
	double snan = __builtin_nans("");
	double r;
	float rf;

	/* fromfp and friends */
	CHECK(fromfp(2.5, FP_INT_TONEAREST, 32) == 2.0 && fromfp(2.5, FP_INT_TONEARESTFROMZERO, 32) == 3.0 && fromfp(-2.5, FP_INT_UPWARD, 2) == -2.0);
	CHECK(isnan(fromfp(2.0, FP_INT_DOWNWARD, 2)) && fromfp(1.0, FP_INT_DOWNWARD, 2) == 1.0 && fromfp(-2.0, FP_INT_DOWNWARD, 2) == -2.0 && isnan(fromfp(-2.5, FP_INT_DOWNWARD, 2)));
	CHECK(isnan(fromfp(1.0, FP_INT_TOWARDZERO, 0)) && isnan(fromfp(INFINITY, FP_INT_UPWARD, 64)) && isnan(fromfp(qnan, FP_INT_UPWARD, 64)));
	CHECK(ufromfp(3.7, FP_INT_DOWNWARD, 2) == 3.0 && isnan(ufromfp(3.7, FP_INT_UPWARD, 2)) && isnan(ufromfp(-1.0, FP_INT_UPWARD, 8)) && ufromfp(-0.5, FP_INT_UPWARD, 8) == 0);
	CHECK(fromfp(0x1p70, FP_INT_TOWARDZERO, UINT_MAX) == 0x1p70 && isnan(fromfp(0x1p63, FP_INT_TOWARDZERO, 64)) && fromfp(-0x1p63, FP_INT_TOWARDZERO, 64) == -0x1p63);
	feclearexcept(FE_ALL_EXCEPT);
	CHECK(fromfp(2.5, FP_INT_UPWARD, 8) == 3.0 && (fetestexcept(FE_ALL_EXCEPT) & INEXACT) == 0);
	CHECK(fromfpx(2.5, FP_INT_UPWARD, 8) == 3.0 && (fetestexcept(FE_ALL_EXCEPT) & INEXACT) == (FE_ALL_EXCEPT & INEXACT));
	CHECK(fromfpf(2.5f, FP_INT_DOWNWARD, 8) == 2.0f);
	CHECK(isnan(ufromfpxf(7.5f, FP_INT_TONEAREST, 3)) && ufromfpxf(6.5f, FP_INT_TONEAREST, 3) == 6.0f && fromfpl(1.5L, FP_INT_TONEAREST, 4) == 2.0L && ufromfpl(9.9L, FP_INT_DOWNWARD, 4) == 9.0L);

	/* nextup, nextdown */
	CHECK(nextup(1.0) == 1.0 + DBL_EPSILON && nextdown(1.0) == 1.0 - DBL_EPSILON / 2 && nextup(-0.0) == 0x1p-1074 && nextdown(0.0) == -0x1p-1074);
	CHECK(nextup(DBL_MAX) == INFINITY && nextup(INFINITY) == INFINITY && nextup(-INFINITY) == -DBL_MAX && nextup(-0x1p-1074) == 0 && signbit(nextup(-0x1p-1074)));
	CHECK(nextupf(1.0f) == 1.0f + FLT_EPSILON && nextdownf(-FLT_MAX) == -INFINITY && isnan(nextup(qnan)) && nextupl(0.0L) == 0x1p-1074L && nextdownl(1.0L) == 1.0L - DBL_EPSILON / 2);

	/* fmaximum family: NaN and signed-zero rules */
	CHECK(isnan(fmaximum(qnan, 1.0)) && fmaximum(-0.0, 0.0) == 0 && !signbit(fmaximum(-0.0, 0.0)) && signbit(fminimum(0.0, -0.0)) && isnan(fminimum(1.0, qnan)));
	CHECK(fmaximum_num(qnan, 1.0) == 1.0 && fmaximum_num(1.0, qnan) == 1.0 && isnan(fmaximum_num(qnan, qnan)) && fminimum_num(qnan, -2.0) == -2.0 && !signbit(fmaximum_num(-0.0, 0.0)));
	CHECK(fmaximum_mag(-3.0, 2.0) == -3.0 && fminimum_mag(-3.0, 2.0) == 2.0 && fmaximum_mag(-2.0, 2.0) == 2.0 && fminimum_mag(-2.0, 2.0) == -2.0 && isnan(fmaximum_mag(qnan, 1.0)));
	CHECK(fmaximum_mag_num(qnan, -5.0) == -5.0 && fminimum_mag_num(-5.0, 4.0) == 4.0 && fmaximumf(1.0f, 2.0f) == 2.0f && fminimum_numf(NAN, 3.0f) == 3.0f && fmaximum_mag_numl(-1.0L, 0.5L) == -1.0L);
	CHECK(fmax(-0.0, 0.0) == 0 && !signbit(fmax(-0.0, 0.0)) && signbit(fmin(0.0, -0.0)) && fmax(qnan, 1.0) == 1.0 && fminf(2.0f, NAN) == 2.0f);
	feclearexcept(FE_ALL_EXCEPT);
	CHECK(fmaximum_num(snan, 1.0) == 1.0 && (fetestexcept(FE_ALL_EXCEPT) & INVALID) == (FE_ALL_EXCEPT & INVALID));
	feclearexcept(FE_ALL_EXCEPT);
	CHECK(isnan(fmax(snan, 1.0)) && (fetestexcept(FE_ALL_EXCEPT) & INVALID) == (FE_ALL_EXCEPT & INVALID));

	/* roundeven */
	CHECK(roundeven(0.5) == 0 && roundeven(1.5) == 2.0 && roundeven(2.5) == 2.0 && roundeven(-2.5) == -2.0 && signbit(roundeven(-0.5)) && roundeven(0.75) == 1.0);
	CHECK(roundevenf(3.5f) == 4.0f && roundevenl(-3.5L) == -4.0L && roundeven(0x1p52 + 1) == 0x1p52 + 1 && roundeven(4503599627370495.5) == 4503599627370496.0);

	/* Remainders: ties go to the even quotient */
	int quotient;

	CHECK(remainder(5.0, 2.0) == 1.0 && remainder(7.0, 2.0) == -1.0 && remainder(3.0, 2.0) == -1.0 && remainder(1.0, 2.0) == 1.0 && remainder(-1.5, 1.0) == 0.5);
	CHECK(remquo(7.0, 2.0, &quotient) == -1.0 && quotient == 4 && remquo(-5.0, 2.0, &quotient) == -1.0 && quotient == -2 && remquo(1.5, -1.0, &quotient) == -0.5 && quotient == -2);
	CHECK(fmod(-7.0, 2.0) == -1.0 && fmod(7.0, -2.0) == 1.0 && signbit(fmod(-4.0, 2.0)) && remainderf(5.0f, 2.0f) == 1.0f && fmodl(5.5L, 2.0L) == 1.5L);

	/* lrint and lround use the width of long */
	CHECK(sizeof(long) == 8 ? lrint(0x1p40) == 0x10000000000L && lround(-0x1p40) == -0x10000000000L : lrint(0x1p40) == LONG_MIN && lround(0x1p31) == LONG_MIN);
	CHECK(llrint(0x1p62) == 0x4000000000000000LL && llround(-2.5) == -3 && lround(2.5) == 3 && lrint(2.5) == 2);

	/* llogb, nan */
	CHECK(llogb(8.0) == 3 && llogb(0x1p-1074) == -1074 && llogb(0.0) == FP_LLOGB0 && llogb(qnan) == FP_LLOGBNAN && llogb(INFINITY) == LONG_MAX && llogbf(0.5f) == -1);
	CHECK(isnan(nan("")) && isnan(nan("1234")) && isnan(nanf("x")) && isnan(nanl("")) && !signbit(nan("")) && !issignaling(nan("")));

	/* Classification, including the C23 macros */
	CHECK(issignaling(snan) && !issignaling(qnan) && !issignaling(1.0) && issubnormal(0x1p-1030) && !issubnormal(0x1p-1022) && iszero(-0.0) && !iszero(0x1p-1074));
	CHECK(iscanonical(1.0) && iscanonical(qnan) && issignaling(__builtin_nansf("")) && issubnormal(0x1p-130f) && fpclassify(0x1p-1074) == FP_SUBNORMAL && fpclassify(-INFINITY) == FP_INFINITE);
	feclearexcept(FE_ALL_EXCEPT);
	CHECK(!isgreater(qnan, 1.0) && isunordered(qnan, 1.0) && islessgreater(1.0, 2.0) && fetestexcept(FE_ALL_EXCEPT) == 0);
	CHECK(iseqsig(1.0, 1.0) && !iseqsig(1.0f, 2.0) && iseqsig(0.0, -0.0L) && fetestexcept(FE_ALL_EXCEPT) == 0);
	CHECK(!iseqsig(qnan, qnan) && (fetestexcept(FE_ALL_EXCEPT) & INVALID) == (FE_ALL_EXCEPT & INVALID));

	/* canonicalize, total order, payloads */
	CHECK(canonicalize(&r, &snan) == 0 && isnan(r) && !issignaling(r));
	CHECK(canonicalizef(&rf, &(float){ -0.0f }) == 0 && rf == 0 && signbit(rf));
	CHECK(totalorder(&(double){ -0.0 }, &(double){ 0.0 }) && !totalorder(&(double){ 0.0 }, &(double){ -0.0 }) && totalorder(&(double){ -INFINITY }, &(double){ -1.0 }));
	CHECK(totalorder(&(double){ 1.0 }, &qnan) && !totalorder(&qnan, &(double){ INFINITY }) && totalorder(&snan, &qnan) && totalorder(&(double){ -qnan }, &(double){ -INFINITY }));
	CHECK(!totalorder(&(double){ -1.0 }, &(double){ -2.0 }) && totalorder(&(double){ -2.0 }, &(double){ -1.0 }) && totalorderf(&(float){ -2.0f }, &(float){ -1.0f }));
	CHECK(totalordermag(&(double){ -1.0 }, &(double){ 2.0 }) && !totalordermag(&(double){ -3.0 }, &(double){ 2.0 }) && totalorderf(&(float){ 1.0f }, &(float){ 2.0f }) && totalorderl(&(long double){ 1.0L }, &(long double){ 1.0L }));
	CHECK(setpayload(&r, 42.0) == 0 && isnan(r) && !issignaling(r) && getpayload(&r) == 42.0 && getpayload(&(double){ 1.0 }) == -1.0);
	CHECK(setpayload(&r, 0x1p51) != 0 && r == 0 && !signbit(r) && setpayload(&r, 1.5) != 0 && setpayload(&r, -1.0) != 0 && setpayload(&r, qnan) != 0);
	CHECK(setpayloadsig(&r, 7.0) == 0 && issignaling(r) && setpayloadsig(&r, 0.0) != 0);
	/* getpayload is quiet, even for a signaling NaN */
	CHECK(setpayloadsig(&r, 7.0) == 0 && feclearexcept(FE_ALL_EXCEPT) == 0 && getpayload(&r) == 7.0 && fetestexcept(FE_ALL_EXCEPT) == 0);
	CHECK(setpayloadf(&rf, 0x1p22f - 1) == 0 && getpayloadf(&rf) == 0x1p22f - 1 && setpayloadsigf(&rf, 0x1p22f) != 0 && setpayloadl(&(long double){ 0 }, 3.0L) == 0);
}

/* <fenv.h> additions of C23 (exception flags are canonical values; a core without them has FE_ALL_EXCEPT 0) */
static void check_fenv(void)
{
	fexcept_t flags;
	femode_t mode;
	fenv_t env;

	CHECK(feclearexcept(FE_ALL_EXCEPT) == 0 && fesetexcept(FE_ALL_EXCEPT & INEXACT) == 0 && fetestexcept(FE_ALL_EXCEPT) == (int)(FE_ALL_EXCEPT & INEXACT));
	CHECK(fegetexceptflag(&flags, FE_ALL_EXCEPT) == 0 && fetestexceptflag(&flags, FE_ALL_EXCEPT) == (int)(FE_ALL_EXCEPT & INEXACT));
	CHECK(feclearexcept(FE_ALL_EXCEPT) == 0 && fetestexceptflag(&flags, FE_ALL_EXCEPT & INEXACT) == (int)(FE_ALL_EXCEPT & INEXACT) && fetestexcept(FE_ALL_EXCEPT) == 0);

	/* Modes travel without the flags; FE_DFL_MODE and FE_DFL_ENV restore start-up state */
	bool directed = fesetround(1) == 0;

	CHECK(fegetmode(&mode) == 0 && fesetround(FE_TONEAREST) == 0 && fesetexcept(FE_ALL_EXCEPT) == 0);
	CHECK(fesetmode(&mode) == 0 && fegetround() == (directed ? 1 : FE_TONEAREST) && fetestexcept(FE_ALL_EXCEPT) == FE_ALL_EXCEPT);
	CHECK(fesetmode(FE_DFL_MODE) == 0 && fegetround() == FE_TONEAREST && fetestexcept(FE_ALL_EXCEPT) == FE_ALL_EXCEPT);
	CHECK(feholdexcept(&env) == 0 && fetestexcept(FE_ALL_EXCEPT) == 0 && feupdateenv(&env) == 0 && fetestexcept(FE_ALL_EXCEPT) == FE_ALL_EXCEPT);
	/* feupdateenv keeps the exceptions raised meanwhile, beside the saved ones */
	CHECK(feclearexcept(FE_ALL_EXCEPT) == 0 && fesetexcept(FE_ALL_EXCEPT & INVALID) == 0 && feholdexcept(&env) == 0 && fesetexcept(FE_ALL_EXCEPT & INEXACT) == 0);
	CHECK(feupdateenv(&env) == 0 && fetestexcept(FE_ALL_EXCEPT) == (int)(FE_ALL_EXCEPT & (INVALID | INEXACT)));
	CHECK(fesetenv(FE_DFL_ENV) == 0 && fetestexcept(FE_ALL_EXCEPT) == 0 && fegetround() == FE_TONEAREST);
	CHECK(fesetround(4) != 0 && fesetround(-1) != 0 && FLT_ROUNDS == 1);
	CHECK(!directed || (fesetround(1) == 0 && FLT_ROUNDS == 2 && fesetround(2) == 0 && FLT_ROUNDS == 3 && fesetround(3) == 0 && FLT_ROUNDS == 0 && fesetround(FE_TONEAREST) == 0));
}

int main(void)
{
	check_vectors();
	check_special();
	check_c23();
	check_fenv();

	if (failures == 0)
		puts("pass: libm");

	return failures == 0 ? 0 : 1;
}
