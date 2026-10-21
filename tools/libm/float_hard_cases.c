/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * float_hard_cases.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * The exhaustive search for the hard cases of the float functions that are
 * their double function rounded to float (PLAN D98,
 * docs/research/float-hard-cases.md), a host program (glibc, MPFR, threads)
 * built by tools/scripts/float-hard-cases against rtoskit-libc's x86_64
 * libc.a with every symbol prefixed rt_.
 *
 * For each of the 2^32 arguments: the double result y = f(x) can be on the
 * wrong side of a float halfway point only within its own error (one double
 * ulp, D47), so MPFR (correctly rounded to float, subnormals included)
 * decides only the arguments whose y lies within 2^-22 float ulps (2^7
 * double ulps) of one, and those whose float result is not y rounded. Every
 * argument the float function rounds wrongly is printed as
 * "<name> <x> <correct> <result> <direction>", direction being the side of
 * the correct result on which the exact value lies.
 */

#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <mpfr.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define THREADS 4

double rt_acos(double x);
float rt_acosf(float x);
double rt_acosh(double x);
float rt_acoshf(float x);
double rt_acospi(double x);
float rt_acospif(float x);
double rt_asin(double x);
float rt_asinf(float x);
double rt_asinh(double x);
float rt_asinhf(float x);
double rt_asinpi(double x);
float rt_asinpif(float x);
double rt_atan(double x);
float rt_atanf(float x);
double rt_atanh(double x);
float rt_atanhf(float x);
double rt_atanpi(double x);
float rt_atanpif(float x);
double rt_cbrt(double x);
float rt_cbrtf(float x);
double rt_cos(double x);
float rt_cosf(float x);
double rt_cosh(double x);
float rt_coshf(float x);
double rt_cospi(double x);
float rt_cospif(float x);
double rt_erfc(double x);
float rt_erfcf(float x);
double rt_erf(double x);
float rt_erff(float x);
double rt_exp10(double x);
float rt_exp10f(float x);
double rt_exp10m1(double x);
float rt_exp10m1f(float x);
double rt_exp2(double x);
float rt_exp2f(float x);
double rt_exp2m1(double x);
float rt_exp2m1f(float x);
double rt_exp(double x);
float rt_expf(float x);
double rt_expm1(double x);
float rt_expm1f(float x);
double rt_lgamma(double x);
float rt_lgammaf(float x);
double rt_log10(double x);
float rt_log10f(float x);
double rt_log10p1(double x);
float rt_log10p1f(float x);
double rt_log1p(double x);
float rt_log1pf(float x);
double rt_log2(double x);
float rt_log2f(float x);
double rt_log2p1(double x);
float rt_log2p1f(float x);
double rt_log(double x);
float rt_logf(float x);
double rt_rsqrt(double x);
float rt_rsqrtf(float x);
double rt_sin(double x);
float rt_sinf(float x);
double rt_sinh(double x);
float rt_sinhf(float x);
double rt_sinpi(double x);
float rt_sinpif(float x);
double rt_tan(double x);
float rt_tanf(float x);
double rt_tanh(double x);
float rt_tanhf(float x);
double rt_tanpi(double x);
float rt_tanpif(float x);
double rt_tgamma(double x);
float rt_tgammaf(float x);

static int lgamma_exact(mpfr_t result, const mpfr_t x, mpfr_rnd_t rnd)
{
	int sign;

	return mpfr_lgamma(result, &sign, x, rnd);
}

struct function {
	const char *name;
	double (*d)(double);
	float (*f)(float);
	int (*exact)(mpfr_t, const mpfr_t, mpfr_rnd_t);
};

#define FUNCTION(n, m)               \
	{                                \
		#n "f", rt_##n, rt_##n##f, m \
	}

static const struct function functions[] = {
	FUNCTION(acos, mpfr_acos),
	FUNCTION(acosh, mpfr_acosh),
	FUNCTION(acospi, mpfr_acospi),
	FUNCTION(asin, mpfr_asin),
	FUNCTION(asinh, mpfr_asinh),
	FUNCTION(asinpi, mpfr_asinpi),
	FUNCTION(atan, mpfr_atan),
	FUNCTION(atanh, mpfr_atanh),
	FUNCTION(atanpi, mpfr_atanpi),
	FUNCTION(cbrt, mpfr_cbrt),
	FUNCTION(cos, mpfr_cos),
	FUNCTION(cosh, mpfr_cosh),
	FUNCTION(cospi, mpfr_cospi),
	FUNCTION(erfc, mpfr_erfc),
	FUNCTION(erf, mpfr_erf),
	FUNCTION(exp10, mpfr_exp10),
	FUNCTION(exp10m1, mpfr_exp10m1),
	FUNCTION(exp2, mpfr_exp2),
	FUNCTION(exp2m1, mpfr_exp2m1),
	FUNCTION(exp, mpfr_exp),
	FUNCTION(expm1, mpfr_expm1),
	FUNCTION(lgamma, lgamma_exact),
	FUNCTION(log10, mpfr_log10),
	FUNCTION(log10p1, mpfr_log10p1),
	FUNCTION(log1p, mpfr_log1p),
	FUNCTION(log2, mpfr_log2),
	FUNCTION(log2p1, mpfr_log2p1),
	FUNCTION(log, mpfr_log),
	FUNCTION(rsqrt, mpfr_rec_sqrt),
	FUNCTION(sin, mpfr_sin),
	FUNCTION(sinh, mpfr_sinh),
	FUNCTION(sinpi, mpfr_sinpi),
	FUNCTION(tan, mpfr_tan),
	FUNCTION(tanh, mpfr_tanh),
	FUNCTION(tanpi, mpfr_tanpi),
	FUNCTION(tgamma, mpfr_gamma),
};

static const struct function *function;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static uint64_t checked;

static uint32_t bits_of(float x)
{
	uint32_t bits;

	memcpy(&bits, &x, sizeof(bits));
	return bits;
}

/* f(x) correctly rounded to float; *direction the side of it on which the exact value lies */
static float correct(mpfr_t x, mpfr_t result, float v, int *direction)
{
	int inexact;
	float value;

	mpfr_set_flt(x, v, MPFR_RNDN);
	mpfr_set_emin(-148);
	mpfr_set_emax(128);
	inexact = function->exact(result, x, MPFR_RNDN);
	inexact = mpfr_check_range(result, inexact, MPFR_RNDN);
	inexact = mpfr_subnormalize(result, inexact, MPFR_RNDN);
	value = mpfr_get_flt(result, MPFR_RNDN);
	mpfr_set_emin(mpfr_get_emin_min());
	mpfr_set_emax(mpfr_get_emax_max());
	/* The rounded value is above the exact one when inexact > 0: the exact value lies below it */
	*direction = inexact > 0 ? -1 : 1;
	return value;
}

/* Whether the double result lies within 2^-22 float ulps of a float halfway point */
static bool near_half(double y)
{
	double k;
	int exponent;

	if (!isfinite(y) || fabs(y) > FLT_MAX || y == 0)
		return false;
	if (fabs(y) < FLT_MIN)
		k = fabs(y) * 0x1p149;
	else
		k = frexp(fabs(y), &exponent) * 0x1p24;
	return fabs(k - floor(k) - 0.5) < 0x1p-22;
}

static void *search(void *context)
{
	uint64_t first = (uintptr_t)context * ((1ULL << 32) / THREADS);
	uint64_t last = first + (1ULL << 32) / THREADS;
	uint64_t count = 0;
	mpfr_t x;
	mpfr_t result;

	mpfr_init2(x, 24);
	mpfr_init2(result, 24);
	for (uint64_t bits = first; bits < last; ++bits) {
		uint32_t u = (uint32_t)bits;
		float v;
		double y;
		float r;
		float c;
		int direction;

		memcpy(&v, &u, sizeof(v));
		if (isnan(v))
			continue;
		y = function->d(v);
		r = function->f(v);
		if (bits_of(r) == bits_of((float)y) && !near_half(y))
			continue;
		++count;
		c = correct(x, result, v, &direction);
		if (bits_of(c) != bits_of(r) && !(isnan(c) && isnan(r))) {
			pthread_mutex_lock(&lock);
			printf("%s %a %a %a %+d\n", function->name, (double)v, (double)c, (double)r, direction);
			fflush(stdout);
			pthread_mutex_unlock(&lock);
		}
	}
	mpfr_clears(x, result, (mpfr_ptr)0);
	mpfr_free_cache();
	pthread_mutex_lock(&lock);
	checked += count;
	pthread_mutex_unlock(&lock);
	return NULL;
}

int main(int argc, char *argv[])
{
	for (size_t i = 0; i < sizeof(functions) / sizeof(functions[0]); ++i) {
		pthread_t threads[THREADS];
		bool selected = argc < 2;

		for (int k = 1; k < argc; ++k)
			selected |= strcmp(argv[k], functions[i].name) == 0;
		if (!selected)
			continue;
		function = &functions[i];
		checked = 0;
		for (uintptr_t k = 0; k < THREADS; ++k)
			pthread_create(&threads[k], NULL, search, (void *)k);
		for (int k = 0; k < THREADS; ++k)
			pthread_join(threads[k], NULL);
		printf("done %s %" PRIu64 "\n", function->name, checked);
		fflush(stdout);
	}
	return 0;
}
