/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * complex_float.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "internal/complex.h"

/*
 * The float complex functions against the double functions on the widened
 * arguments. The error of a result is normwise (PLAN D49): the larger
 * difference of the parts, in float ulps of the larger exact part. D49
 * allows 2; every core measures below 0.52, the double wrappers of
 * armv7e-m-dp included, so that is the bound. Where the float complex layer
 * is linked (COMPLEX_FLOAT_LAYER), each part must also be within 0.52 ulp of
 * its own exact value and at most one result in a thousand (plus one) may
 * differ from the correctly rounded one (PLAN D65); for cpowf the parts'
 * errors are reported only, as a w with subnormal parts gives a phase made of
 * subnormal products. A part whose exact value
 * is beyond FLT_MAX must be an infinity or FLT_MAX of its sign. Special operands (zeros, infinities, NaN, the
 * extremes) must give the double function's zeros, infinities and NaNs
 * exactly, signs included. On x86_64 the test links the float complex layer
 * of the single-precision cores (libc/complex/sp-nearest, PLAN D65) and runs
 * COMPLEX_FLOAT_SAMPLES arguments per function natively; on the Arm boards it
 * measures the library as built.
 */

static int failures;

/* The float complex layer is linked: the componentwise bounds apply */
static constexpr bool layer = COMPLEX_FLOAT_LAYER != 0;

static float cabsf_complex(float complex z)
{
	return cabsf(z);
}

static double cabs_complex(double complex z)
{
	return cabs(z);
}

static float cargf_complex(float complex z)
{
	return cargf(z);
}

static double carg_complex(double complex z)
{
	return carg(z);
}

/* How the parts of the samples are drawn: anywhere in [low, high], or near the unit circle */
enum kind {
	ANY,
	UNIT,
};

struct unary {
	const char *name;
	float complex (*function)(float complex);
	double complex (*reference)(double complex);
	float (*real_function)(float complex);
	double (*real_reference)(double complex);
	float low_re;
	float high_re;
	float low_im;
	float high_im;
	enum kind kind;
};

static const struct unary unary[] = {
	{ "cexpf", cexpf, cexp, nullptr, nullptr, -120.0f, 100.0f, -FLT_MAX, FLT_MAX, ANY },
	{ "clogf", clogf, clog, nullptr, nullptr, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, ANY },
	{ "clogf near |z| = 1", clogf, clog, nullptr, nullptr, -1.0f, 1.0f, -1.0f, 1.0f, UNIT },
	{ "csqrtf", csqrtf, csqrt, nullptr, nullptr, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, ANY },
	{ "csinhf", csinhf, csinh, nullptr, nullptr, -110.0f, 110.0f, -FLT_MAX, FLT_MAX, ANY },
	{ "ccoshf", ccoshf, ccosh, nullptr, nullptr, -110.0f, 110.0f, -FLT_MAX, FLT_MAX, ANY },
	{ "ctanhf", ctanhf, ctanh, nullptr, nullptr, -20.0f, 20.0f, -FLT_MAX, FLT_MAX, ANY },
	{ "csinf", csinf, csin, nullptr, nullptr, -FLT_MAX, FLT_MAX, -110.0f, 110.0f, ANY },
	{ "ccosf", ccosf, ccos, nullptr, nullptr, -FLT_MAX, FLT_MAX, -110.0f, 110.0f, ANY },
	{ "ctanf", ctanf, ctan, nullptr, nullptr, -FLT_MAX, FLT_MAX, -20.0f, 20.0f, ANY },
	{ "cacosf", cacosf, cacos, nullptr, nullptr, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, ANY },
	{ "casinf", casinf, casin, nullptr, nullptr, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, ANY },
	{ "catanf", catanf, catan, nullptr, nullptr, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, ANY },
	{ "cacoshf", cacoshf, cacosh, nullptr, nullptr, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, ANY },
	{ "casinhf", casinhf, casinh, nullptr, nullptr, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, ANY },
	{ "catanhf", catanhf, catanh, nullptr, nullptr, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, ANY },
	{ "catanhf near 1", catanhf, catanh, nullptr, nullptr, 0.5f, 2.0f, -1.0f, 1.0f, ANY },
	{ "cacosf near the cut", cacosf, cacos, nullptr, nullptr, -2.0f, 2.0f, -0x1p-10f, 0x1p-10f, ANY },
	{ "casinhf near the cut", casinhf, casinh, nullptr, nullptr, -0x1p-10f, 0x1p-10f, -2.0f, 2.0f, ANY },
	{ "cabsf", nullptr, nullptr, cabsf_complex, cabs_complex, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, ANY },
	{ "cargf", nullptr, nullptr, cargf_complex, carg_complex, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, ANY },
};

static uint32_t next(uint32_t *state)
{
	*state ^= *state << 13;
	*state ^= *state >> 17;
	*state ^= *state << 5;
	return *state;
}

static float from_bits(uint32_t bits)
{
	float x;

	memcpy(&x, &bits, sizeof(x));
	return x;
}

static uint32_t bits_of(float x)
{
	uint32_t bits;

	memcpy(&bits, &x, sizeof(bits));
	return bits;
}

/* Random floats in [low, high]: random bit patterns (every binade) and uniform values, alternately */
static float sample(uint32_t *state, float low, float high, int i)
{
	float x;

	if ((i & 1) != 0) {
		float u = (float)(next(state) >> 8) * 0x1p-24f;

		return low * (1.0f - u) + high * u;
	}
	do {
		x = from_bits(next(state));
	} while (!(x >= low && x <= high));

	return x;
}

/* The float ulp of a positive finite magnitude (2^-149 for the subnormals) */
static double ulp_of(double magnitude)
{
	int exponent;

	frexp(magnitude, &exponent);
	if (exponent < -125)
		exponent = -125;

	return ldexp(1.0, exponent - 24);
}

/* The difference of one part in absolute terms; an exact part past FLT_MAX accepts infinity or FLT_MAX of its sign */
static double part_error(float got, double exact)
{
	if (fabs(exact) > FLT_MAX)
		return (isinf(got) || fabsf(got) == FLT_MAX) && !signbit(got) == !signbit(exact) ? 0.0 : INFINITY;
	if (isnan(got) || isinf(got))
		return INFINITY;

	return fabs((double)got - exact);
}

/* The normwise error in float ulps, or 0 when the exact result is not finite and the parts match it */
static double complex_error(float complex got, double complex exact)
{
	double rr = __libc_re(exact);
	double ri = __libc_im(exact);
	double magnitude = fmax(fabs(rr), fabs(ri));

	if (!isfinite(rr) || !isfinite(ri)) {
		float gr = __libc_ref(got);
		float gi = __libc_imf(got);
		bool real_ok = isnan(rr) ? isnan(gr) : isinf(rr) ? (float)rr == gr :
														   part_error(gr, rr) <= 2.0 * ulp_of(fmax(fabs(rr), 0x1p-149));
		bool imag_ok = isnan(ri) ? isnan(gi) : isinf(ri) ? (float)ri == gi :
														   part_error(gi, ri) <= 2.0 * ulp_of(fmax(fabs(ri), 0x1p-149));

		return real_ok && imag_ok ? 0.0 : INFINITY;
	}
	if (magnitude == 0)
		return __libc_ref(got) == 0 && __libc_imf(got) == 0 ? 0.0 : INFINITY;

	return fmax(part_error(__libc_ref(got), rr), part_error(__libc_imf(got), ri)) / ulp_of(fmin(magnitude, FLT_MAX));
}

/* The larger error of the two parts, each in float ulps of its own exact value (informational: the bound is normwise) */
static double part_ulps(float complex got, double complex exact)
{
	double rr = __libc_re(exact);
	double ri = __libc_im(exact);
	double worst = 0;

	if (isfinite(rr) && fabs(rr) <= FLT_MAX && rr != 0)
		worst = fmax(worst, part_error(__libc_ref(got), rr) / ulp_of(fabs(rr)));
	if (isfinite(ri) && fabs(ri) <= FLT_MAX && ri != 0)
		worst = fmax(worst, part_error(__libc_imf(got), ri) / ulp_of(fabs(ri)));

	return worst;
}

static float complex evaluate(const struct unary *test, float complex z)
{
	if (test->function != nullptr)
		return test->function(z);

	return CMPLXF(test->real_function(z), 0.0f);
}

static double complex reference(const struct unary *test, float complex z)
{
	double complex wide = CMPLX(__libc_ref(z), __libc_imf(z));

	if (test->reference != nullptr)
		return test->reference(wide);

	return CMPLX(test->real_reference(wide), 0.0);
}

/* The parts that are zero, infinite or NaN in the reference must match it exactly, signs included */
static bool same_specials(float complex got, double complex exact)
{
	float parts[2] = { __libc_ref(got), __libc_imf(got) };
	double wanted[2] = { __libc_re(exact), __libc_im(exact) };

	for (int i = 0; i < 2; ++i) {
		if (isnan(wanted[i]) != isnan(parts[i]))
			return false;
		if ((wanted[i] == 0 || isinf(wanted[i])) && bits_of(parts[i]) != bits_of((float)wanted[i]))
			return false;
	}

	return true;
}

static const float specials[] = { 0.0f, -0.0f, 1.0f, -1.0f, 0.5f, -2.0f, 3.0f, 0x1p-149f, -0x1p-140f, FLT_MAX, -FLT_MAX, INFINITY, -INFINITY, NAN };
#define SPECIALS (sizeof(specials) / sizeof(specials[0]))

static void report_failure(const char *name, float complex z, float complex got, double complex exact)
{
	char message[200];

	if (failures >= 40)
		return;
	snprintf(message, sizeof(message), "fail: %s(%a%+ai) = %a%+ai, exact %a%+ai\n", name, (double)__libc_ref(z), (double)__libc_imf(z), (double)__libc_ref(got), (double)__libc_imf(got), __libc_re(exact), __libc_im(exact));
	fputs(message, stdout);
}

static void check_unary(const struct unary *test)
{
	uint32_t state = 0x2545f491;
	double worst = 0;
	double worst_part = 0;
	float complex worst_z = 0;
	float complex worst_part_z = 0;
	long misrounded = 0;
	char message[200];

	for (size_t i = 0; i < SPECIALS * SPECIALS; ++i) {
		float complex z = CMPLXF(specials[i / SPECIALS], specials[i % SPECIALS]);
		float complex got = evaluate(test, z);
		double complex exact = reference(test, z);

		if (!same_specials(got, exact) || complex_error(got, exact) >= 0.52) {
			report_failure(test->name, z, got, exact);
			++failures;
		}
	}

	for (long i = 0; i < COMPLEX_FLOAT_SAMPLES; ++i) {
		float x = sample(&state, test->low_re, test->high_re, (int)i);
		float y = sample(&state, test->low_im, test->high_im, (int)i >> 1);
		float complex z;
		float complex got;
		double complex exact;
		double error;

		if (test->kind == UNIT) {
			/* |z| within a few ulps of 1, where log|z| is tiny */
			y = copysignf(sqrtf(fabsf(1.0f - x * x)), y);
			for (uint32_t steps = next(&state) % 8; steps > 0; --steps)
				y = nextafterf(y, (steps & 1) != 0 ? INFINITY : -INFINITY);
		}
		z = CMPLXF(x, y);
		got = evaluate(test, z);
		exact = reference(test, z);
		error = complex_error(got, exact);
		if (__libc_ref(got) != (float)__libc_re(exact) || __libc_imf(got) != (float)__libc_im(exact))
			++misrounded;
		if (error >= 0.52 || (layer && part_ulps(got, exact) >= 0.52)) {
			report_failure(test->name, z, got, exact);
			++failures;
		}
		if (error > worst) {
			worst = error;
			worst_z = z;
		}
		if (part_ulps(got, exact) > worst_part) {
			worst_part = part_ulps(got, exact);
			worst_part_z = z;
		}
	}

	snprintf(message, sizeof(message), "%s: max %.3f ulp at %a%+ai, %ld of %ld not correctly rounded\n", test->name, worst, (double)__libc_ref(worst_z), (double)__libc_imf(worst_z), misrounded, (long)COMPLEX_FLOAT_SAMPLES);
	fputs(message, stdout);
	snprintf(message, sizeof(message), "%s: parts max %.3g ulp of their own at %a%+ai\n", test->name, worst_part, (double)__libc_ref(worst_part_z), (double)__libc_imf(worst_part_z));
	if (layer && misrounded > COMPLEX_FLOAT_SAMPLES / 1000 + 1)
		++failures;
	fputs(message, stdout);
}

/*
 * cpowf: Annex G gives cpow no special cases of its own (it may be
 * cexp(w clog z)), so the special operands check w = 0 (1 + i0), a zero z
 * with a positive real w (0) and every in-range result for the small real w
 * below; then z anywhere in a box and w small enough that many results stay
 * finite. A huge or infinite w on |z| near 1 asks for a phase that neither
 * float-float nor the double reference resolves.
 */
static void check_cpowf(void)
{
	static const float exponents[] = { 0.0f, -0.0f, 1.0f, -1.0f, 0.5f, -2.0f, 3.0f };
	uint32_t state = 0x9e3779b9;
	double worst = 0;
	double worst_part = 0;
	long misrounded = 0;
	char message[240];

	for (size_t i = 0; i < SPECIALS * SPECIALS; ++i) {
		for (size_t j = 0; j < sizeof(exponents) / sizeof(exponents[0]); ++j) {
			float complex z = CMPLXF(specials[i / SPECIALS], specials[i % SPECIALS]);
			float complex w = CMPLXF(exponents[j], 0.0f);
			float complex got = cpowf(z, w);
			double complex exact = cpow(CMPLX(__libc_ref(z), __libc_imf(z)), CMPLX(__libc_ref(w), 0.0));
			bool in_range = isfinite(__libc_re(exact)) && isfinite(__libc_im(exact)) && fmax(fabs(__libc_re(exact)), fabs(__libc_im(exact))) <= FLT_MAX;

			if ((exponents[j] == 0 && (__libc_ref(got) != 1.0f || bits_of(__libc_imf(got)) != 0)) || (in_range && complex_error(got, exact) >= 0.52)) {
				snprintf(message, sizeof(message), "fail: cpowf(%a%+ai, %a) = %a%+ai, exact %a%+ai\n", (double)__libc_ref(z), (double)__libc_imf(z), (double)__libc_ref(w), (double)__libc_ref(got), (double)__libc_imf(got), __libc_re(exact), __libc_im(exact));
				if (failures < 40)
					fputs(message, stdout);
				++failures;
			}
		}
	}

	for (long i = 0; i < COMPLEX_FLOAT_SAMPLES; ++i) {
		float complex z = CMPLXF(sample(&state, -100.0f, 100.0f, (int)i), sample(&state, -100.0f, 100.0f, (int)i >> 1));
		float complex w = CMPLXF(sample(&state, -8.0f, 8.0f, (int)i >> 2), sample(&state, -8.0f, 8.0f, (int)i >> 3));
		float complex got = cpowf(z, w);
		double complex exact = cpow(CMPLX(__libc_ref(z), __libc_imf(z)), CMPLX(__libc_ref(w), __libc_imf(w)));
		double error = complex_error(got, exact);

		if (__libc_ref(got) != (float)__libc_re(exact) || __libc_imf(got) != (float)__libc_im(exact))
			++misrounded;
		if (error >= 0.52) {
			snprintf(message, sizeof(message), "fail: cpowf(%a%+ai, %a%+ai)\n", (double)__libc_ref(z), (double)__libc_imf(z), (double)__libc_ref(w), (double)__libc_imf(w));
			if (failures < 40)
				fputs(message, stdout);
			++failures;
		}
		if (error > worst)
			worst = error;
		if (part_ulps(got, exact) > worst_part)
			worst_part = part_ulps(got, exact);
	}

	snprintf(message, sizeof(message), "cpowf: max %.3f ulp, parts max %.3g ulp of their own, %ld of %ld not correctly rounded\n", worst, worst_part, misrounded, (long)COMPLEX_FLOAT_SAMPLES);
	fputs(message, stdout);
	if (layer && misrounded > COMPLEX_FLOAT_SAMPLES / 1000 + 1)
		++failures;
}

int main(void)
{
	for (size_t i = 0; i < sizeof(unary) / sizeof(unary[0]); ++i)
		check_unary(&unary[i]);
	check_cpowf();

	fputs(failures == 0 ? "complex_float: pass\n" : "complex_float: FAIL\n", stdout);
	return failures == 0 ? 0 : 1;
}
