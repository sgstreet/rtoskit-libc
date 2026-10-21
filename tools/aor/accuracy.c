/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * accuracy.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * The accuracy comparison with Arm Optimized Routines (P4.05, PLAN D92), a
 * host program (glibc, MPFR, threads) built by tools/scripts/aor-accuracy.
 * It calls rtoskit-libc's x86_64 functions as rt_<name> (libc.a with every
 * symbol prefixed) and AOR's as aor_<name> (its entry points renamed), and
 * measures both against MPFR at 200 bits:
 *
 *   exhaustive <f>   every float argument: where the two results differ,
 *                    both are checked; where they agree, one argument in
 *                    16 is (the agreement is then correct, or wrong for
 *                    both, which the sample estimates)
 *   sample <f> <n>   n arguments drawn over the function's domain, every
 *                    one checked (doubles, and the binary functions)
 *   modes <f> <n>    the same in the three directed rounding modes
 *   special          special arguments: results and exception flags
 *   records <label> <file> <n>
 *                    one library's results computed on a target
 *                    (tools/aor/arm-sample.c), measured to nearest
 *
 * Each line of output is "<key> <value> ..." for the driver to tabulate.
 * Errors are in ulps of the exact result's format at its magnitude
 * (subnormal ulps below the normal range); "not correctly rounded" means
 * the result differs from the exact value rounded in the current mode.
 */

#include <fenv.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <mpfr.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRECISION 200
#define THREADS 4
#define AGREE_SAMPLE 16

float rt_expf(float x);
float rt_exp2f(float x);
float rt_logf(float x);
float rt_log2f(float x);
float rt_log10f(float x);
float rt_sinf(float x);
float rt_cosf(float x);
float rt_erff(float x);
float rt_powf(float x, float y);
double rt_exp(double x);
double rt_exp2(double x);
double rt_exp10(double x);
double rt_log(double x);
double rt_log2(double x);
double rt_erf(double x);
double rt_pow(double x, double y);
float aor_expf(float x);
float aor_exp2f(float x);
float aor_logf(float x);
float aor_log2f(float x);
float aor_log10f(float x);
float aor_sinf(float x);
float aor_cosf(float x);
float aor_erff(float x);
float aor_powf(float x, float y);
double aor_exp(double x);
double aor_exp2(double x);
double aor_exp10(double x);
double aor_log(double x);
double aor_log2(double x);
double aor_erf(double x);
double aor_pow(double x, double y);

struct function {
	const char *name;
	bool is_float;
	bool binary;
	float (*rt_f)(float);
	float (*aor_f)(float);
	double (*rt_d)(double);
	double (*aor_d)(double);
	float (*rt_f2)(float, float);
	float (*aor_f2)(float, float);
	double (*rt_d2)(double, double);
	double (*aor_d2)(double, double);
	int (*exact)(mpfr_t, const mpfr_t, mpfr_rnd_t);
	int (*exact2)(mpfr_t, const mpfr_t, const mpfr_t, mpfr_rnd_t);
	/* The sampled domain: x (and y) uniform in [low, high] */
	double low;
	double high;
	double low2;
	double high2;
};

#define FLOAT1(n, f, lo, hi)                                                                                             \
	{                                                                                                                    \
		#n, true, false, rt_##n, aor_##n, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, f, nullptr, lo, hi, 0, 0 \
	}
#define DOUBLE1(n, f, lo, hi)                                                                                             \
	{                                                                                                                     \
		#n, false, false, nullptr, nullptr, rt_##n, aor_##n, nullptr, nullptr, nullptr, nullptr, f, nullptr, lo, hi, 0, 0 \
	}

static const struct function functions[] = {
	FLOAT1(expf, mpfr_exp, -104, 89),
	FLOAT1(exp2f, mpfr_exp2, -150, 128),
	FLOAT1(logf, mpfr_log, 0, 1e30),
	FLOAT1(log2f, mpfr_log2, 0, 1e30),
	FLOAT1(log10f, mpfr_log10, 0, 1e30),
	FLOAT1(sinf, mpfr_sin, -1e4, 1e4),
	FLOAT1(cosf, mpfr_cos, -1e4, 1e4),
	FLOAT1(erff, mpfr_erf, -5, 5),
	{ "powf", true, true, nullptr, nullptr, nullptr, nullptr, rt_powf, aor_powf, nullptr, nullptr, nullptr, mpfr_pow, 0, 16, -40, 40 },
	DOUBLE1(exp, mpfr_exp, -746, 710),
	DOUBLE1(exp2, mpfr_exp2, -1075, 1024),
	DOUBLE1(exp10, mpfr_exp10, -324, 308),
	DOUBLE1(log, mpfr_log, 0, 1e300),
	DOUBLE1(log2, mpfr_log2, 0, 1e300),
	DOUBLE1(erf, mpfr_erf, -6, 6),
	{ "pow", false, true, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, rt_pow, aor_pow, nullptr, mpfr_pow, 0, 16, -250, 250 },
};

static const struct function *find(const char *name)
{
	for (size_t i = 0; i < sizeof(functions) / sizeof(functions[0]); ++i)
		if (strcmp(functions[i].name, name) == 0)
			return &functions[i];
	fprintf(stderr, "no function %s\n", name);
	exit(2);
}

static mpfr_rnd_t mpfr_mode(int mode)
{
	switch (mode) {
		case FE_UPWARD:
			return MPFR_RNDU;
		case FE_DOWNWARD:
			return MPFR_RNDD;
		case FE_TOWARDZERO:
			return MPFR_RNDZ;
		default:
			return MPFR_RNDN;
	}
}

/* Per library: the largest error seen, where, and how many results were not correctly rounded */
struct tally {
	double max_error;
	double worst_x;
	double worst_y;
	uint64_t wrong;
	uint64_t checked;
};

struct checker {
	mpfr_t x;
	mpfr_t y;
	mpfr_t exact;
	mpfr_t rounded;
	mpfr_t diff;
};

static void checker_init(struct checker *c)
{
	mpfr_inits2(PRECISION, c->x, c->y, c->exact, c->diff, (mpfr_ptr)0);
	mpfr_init2(c->rounded, 53);
}

static void checker_clear(struct checker *c)
{
	mpfr_clears(c->x, c->y, c->exact, c->rounded, c->diff, (mpfr_ptr)0);
}

/* The exact value of f at (x, y) into c->exact; false when it is not a finite number (no error is measured then) */
static bool exact_value(struct checker *c, const struct function *f, double x, double y)
{
	mpfr_set_d(c->x, x, MPFR_RNDN);
	if (f->binary) {
		mpfr_set_d(c->y, y, MPFR_RNDN);
		f->exact2(c->exact, c->x, c->y, MPFR_RNDN);
	} else {
		f->exact(c->exact, c->x, MPFR_RNDN);
	}
	return mpfr_number_p(c->exact);
}

static bool same_value(double a, double b)
{
	return a == b && signbit(a) == signbit(b);
}

/*
 * The error of result in ulps of the format (24 or 53 bits, subnormals
 * below 2^emin) and whether it is the exact value correctly rounded in
 * mode; results beyond the format's range count as not correctly rounded
 * with an infinite error unless the rounding of the exact value is the same.
 */
static void measure(struct checker *c, const struct function *f, double result, mpfr_rnd_t mode, struct tally *t, double x, double y)
{
	int bits = f->is_float ? 24 : 53;
	mpfr_exp_t emin = f->is_float ? -125 : -1021;
	mpfr_exp_t emax = f->is_float ? 128 : 1024;
	mpfr_exp_t old_emin = mpfr_get_emin();
	mpfr_exp_t old_emax = mpfr_get_emax();
	double correct;
	double error;

	/*
	 * The correctly rounded result, computed by MPFR straight into the format and the
	 * mode, subnormals included: rounding the 200-bit value instead would lose results
	 * like exp(2^-842) upward, which only the ternary value of the evaluation tells
	 * apart from 1
	 */
	mpfr_set_prec(c->rounded, bits);
	mpfr_set_emin(emin - bits + 1);
	mpfr_set_emax(emax);
	int inexact = f->binary ? f->exact2(c->rounded, c->x, c->y, mode) : f->exact(c->rounded, c->x, mode);

	inexact = mpfr_check_range(c->rounded, inexact, mode);
	mpfr_subnormalize(c->rounded, inexact, mode);
	correct = mpfr_get_d(c->rounded, MPFR_RNDN);
	mpfr_set_emin(old_emin);
	mpfr_set_emax(old_emax);

	t->checked++;
	if (mpfr_cmp_d(c->exact, f->is_float ? FLT_MAX : DBL_MAX) > 0 || mpfr_cmp_d(c->exact, f->is_float ? -FLT_MAX : -DBL_MAX) < 0) {
		/* Beyond the format: the correct result is the largest finite value or infinity, by mode; no distance applies */
		error = same_value(result, correct) ? 0 : INFINITY;
		if (error != 0)
			t->wrong++;
	} else if (isnan(result) || (isinf(result) && !isinf(correct))) {
		t->wrong++;
		error = INFINITY;
	} else if (isinf(result)) {
		error = 0;
	} else {
		mpfr_exp_t e = mpfr_zero_p(c->exact) ? emin : mpfr_get_exp(c->exact);

		if (e < emin)
			e = emin;
		mpfr_sub_d(c->diff, c->exact, result, MPFR_RNDN);
		mpfr_abs(c->diff, c->diff, MPFR_RNDN);
		mpfr_mul_2si(c->diff, c->diff, bits - e, MPFR_RNDN);
		error = mpfr_get_d(c->diff, MPFR_RNDU);
		if (result != correct)
			t->wrong++;
	}
	if (error > t->max_error) {
		t->max_error = error;
		t->worst_x = x;
		t->worst_y = y;
	}
}

static uint64_t next_random(uint64_t *state)
{
	/* xorshift64*: arguments only need to be spread, not unpredictable */
	*state ^= *state >> 12;
	*state ^= *state << 25;
	*state ^= *state >> 27;
	return *state * 2685821657736338717ULL;
}

static double uniform(uint64_t *state, double low, double high)
{
	return low + (high - low) * ((double)(next_random(state) >> 11) * 0x1p-53);
}

/*
 * An argument in [low, high]: half the draws uniform, half spread over the
 * binades (random sign where the range allows, exponent and significand bits),
 * so tiny and huge magnitudes are covered as well as the middle.
 */
static double argument(uint64_t *state, const struct function *f, double low, double high)
{
	uint64_t r = next_random(state);
	double v;

	if (r & 1) {
		v = uniform(state, low, high);
		return f->is_float ? (float)v : v;
	}
	double limit = fmax(fabs(low), fabs(high));
	int top = ilogb(limit);
	int bottom = f->is_float ? -149 : -1074;
	int e = bottom + (int)(next_random(state) % (uint64_t)(top - bottom + 1));
	double m = 1.0 + (double)(next_random(state) >> 11) * 0x1p-53;

	v = ldexp(m, e);

	if (low < 0 && (r & 2))
		v = -v;
	if (v < low || v > high)
		v = uniform(state, low, high);
	if (f->is_float)
		v = (float)v;
	return v;
}

struct job {
	const struct function *f;
	uint64_t first;
	uint64_t last;
	uint64_t samples;
	uint64_t seed;
	int mode;
	struct tally rt;
	struct tally aor;
	uint64_t differ;
};

static double call_rt(const struct function *f, double x, double y)
{
	if (f->binary)
		return f->is_float ? f->rt_f2((float)x, (float)y) : f->rt_d2(x, y);
	return f->is_float ? f->rt_f((float)x) : f->rt_d(x);
}

static double call_aor(const struct function *f, double x, double y)
{
	if (f->binary)
		return f->is_float ? f->aor_f2((float)x, (float)y) : f->aor_d2(x, y);
	return f->is_float ? f->aor_f((float)x) : f->aor_d(x);
}

static bool same(double a, double b)
{
	return (isnan(a) && isnan(b)) || (a == b && signbit(a) == signbit(b));
}

static void *exhaustive_worker(void *context)
{
	struct job *job = context;
	struct checker c;

	checker_init(&c);
	for (uint64_t bits = job->first; bits < job->last; ++bits) {
		uint32_t u = (uint32_t)bits;
		float x;

		memcpy(&x, &u, sizeof(x));
		if (isnan(x))
			continue;
		double rt = call_rt(job->f, x, 0);
		double aor = call_aor(job->f, x, 0);
		bool differ = !same(rt, aor);

		if (differ)
			job->differ++;
		if (!differ && (u % AGREE_SAMPLE) != 0)
			continue;
		if (!exact_value(&c, job->f, x, 0))
			continue;
		measure(&c, job->f, rt, MPFR_RNDN, &job->rt, x, 0);
		measure(&c, job->f, aor, MPFR_RNDN, &job->aor, x, 0);
	}
	checker_clear(&c);
	mpfr_free_cache();
	return nullptr;
}

static void *sample_worker(void *context)
{
	struct job *job = context;
	struct checker c;
	uint64_t state = job->seed;
	mpfr_rnd_t mode = mpfr_mode(job->mode);

	checker_init(&c);
	for (uint64_t i = 0; i < job->samples; ++i) {
		double x = argument(&state, job->f, job->f->low, job->f->high);
		double y = job->f->binary ? argument(&state, job->f, job->f->low2, job->f->high2) : 0;

		/* Only the calls run in the directed mode: MPFR and the argument generator assume round to nearest */
		fesetround(job->mode);
		double rt = call_rt(job->f, x, y);
		double aor = call_aor(job->f, x, y);

		fesetround(FE_TONEAREST);

		if (!same(rt, aor))
			job->differ++;
		if (!exact_value(&c, job->f, x, y))
			continue;
		measure(&c, job->f, rt, mode, &job->rt, x, y);
		measure(&c, job->f, aor, mode, &job->aor, x, y);
	}
	checker_clear(&c);
	mpfr_free_cache();
	return nullptr;
}

static void merge(struct tally *into, const struct tally *from)
{
	if (from->max_error > into->max_error) {
		into->max_error = from->max_error;
		into->worst_x = from->worst_x;
		into->worst_y = from->worst_y;
	}
	into->wrong += from->wrong;
	into->checked += from->checked;
}

static const char *mode_name(int mode)
{
	switch (mode) {
		case FE_UPWARD:
			return "upward";
		case FE_DOWNWARD:
			return "downward";
		case FE_TOWARDZERO:
			return "towardzero";
		default:
			return "nearest";
	}
}

static void run(const struct function *f, bool exhaustive, uint64_t samples, int mode, const char *label)
{
	struct job jobs[THREADS];
	pthread_t threads[THREADS];
	struct tally rt = { 0 };
	struct tally aor = { 0 };
	uint64_t differ = 0;
	uint64_t total = exhaustive ? 1ULL << 32 : samples;

	for (int i = 0; i < THREADS; ++i) {
		jobs[i] = (struct job){ .f = f, .first = total * i / THREADS, .last = total * (i + 1) / THREADS, .samples = samples / THREADS, .seed = 0x9e3779b97f4a7c15ULL * (uint64_t)(i + 1) + (uint64_t)mode, .mode = mode };
		pthread_create(&threads[i], nullptr, exhaustive ? exhaustive_worker : sample_worker, &jobs[i]);
	}
	for (int i = 0; i < THREADS; ++i) {
		pthread_join(threads[i], nullptr);
		merge(&rt, &jobs[i].rt);
		merge(&aor, &jobs[i].aor);
		differ += jobs[i].differ;
	}
	printf("%s %s %s arguments %" PRIu64 " differ %" PRIu64 "\n", label, f->name, mode_name(mode), total, differ);
	printf("%s %s %s rt max %.4f at %a %a wrong %" PRIu64 " of %" PRIu64 "\n", label, f->name, mode_name(mode), rt.max_error, rt.worst_x, rt.worst_y, rt.wrong, rt.checked);
	printf("%s %s %s aor max %.4f at %a %a wrong %" PRIu64 " of %" PRIu64 "\n", label, f->name, mode_name(mode), aor.max_error, aor.worst_x, aor.worst_y, aor.wrong, aor.checked);
	fflush(stdout);
}

/* Special arguments: the result and the exceptions each library raises */
struct special {
	const char *name;
	double x;
	double y;
};

static const struct special specials[] = {
	{ "expf", 0.0, 0 },
	{ "expf", -0.0, 0 },
	{ "expf", INFINITY, 0 },
	{ "expf", -INFINITY, 0 },
	{ "expf", NAN, 0 },
	{ "expf", 89.0, 0 },
	{ "expf", -104.0, 0 },
	{ "expf", -0x1.9fe368p6, 0 },
	{ "logf", 0.0, 0 },
	{ "logf", -0.0, 0 },
	{ "logf", -1.0, 0 },
	{ "logf", 1.0, 0 },
	{ "logf", INFINITY, 0 },
	{ "logf", -INFINITY, 0 },
	{ "logf", NAN, 0 },
	{ "logf", 0x1p-149, 0 },
	{ "sinf", 0.0, 0 },
	{ "sinf", -0.0, 0 },
	{ "sinf", INFINITY, 0 },
	{ "sinf", NAN, 0 },
	{ "sinf", 0x1p-149, 0 },
	{ "sinf", 0x1p127, 0 },
	{ "cosf", INFINITY, 0 },
	{ "cosf", 0.0, 0 },
	{ "erff", 0.0, 0 },
	{ "erff", -0.0, 0 },
	{ "erff", INFINITY, 0 },
	{ "erff", -INFINITY, 0 },
	{ "erff", 0x1p-149, 0 },
	{ "powf", 0.0, -1.0 },
	{ "powf", -0.0, -1.0 },
	{ "powf", -1.0, 0.5 },
	{ "powf", -1.0, INFINITY },
	{ "powf", 1.0, NAN },
	{ "powf", NAN, 0.0 },
	{ "powf", 2.0, 128.0 },
	{ "powf", 2.0, -150.0 },
	{ "powf", -2.0, 3.0 },
	{ "powf", INFINITY, -0.5 },
	{ "exp", 0.0, 0 },
	{ "exp", INFINITY, 0 },
	{ "exp", -INFINITY, 0 },
	{ "exp", NAN, 0 },
	{ "exp", 710.0, 0 },
	{ "exp", -746.0, 0 },
	{ "exp", -0x1.6232bdd7abcd2p9, 0 },
	{ "log", 0.0, 0 },
	{ "log", -1.0, 0 },
	{ "log", 1.0, 0 },
	{ "log", INFINITY, 0 },
	{ "log", 0x1p-1074, 0 },
	{ "erf", 0.0, 0 },
	{ "erf", -0.0, 0 },
	{ "erf", INFINITY, 0 },
	{ "erf", 0x1p-1074, 0 },
	{ "pow", 0.0, -1.0 },
	{ "pow", -1.0, 0.5 },
	{ "pow", 2.0, 1024.0 },
	{ "pow", 2.0, -1075.0 },
	{ "pow", -2.0, 3.0 },
	{ "pow", 1.0, NAN },
	{ "pow", NAN, 0.0 },
};

static void flags_text(int raised, char *text)
{
	text[0] = 0;
	if (raised & FE_INVALID)
		strcat(text, "I");
	if (raised & FE_DIVBYZERO)
		strcat(text, "Z");
	if (raised & FE_OVERFLOW)
		strcat(text, "O");
	if (raised & FE_UNDERFLOW)
		strcat(text, "U");
	if (raised & FE_INEXACT)
		strcat(text, "X");
	if (!text[0])
		strcat(text, "-");
}

static void run_special(void)
{
	for (size_t i = 0; i < sizeof(specials) / sizeof(specials[0]); ++i) {
		const struct special *s = &specials[i];
		const struct function *f = find(s->name);
		char rt_flags[8];
		char aor_flags[8];
		double rt;
		double aor;

		feclearexcept(FE_ALL_EXCEPT);
		rt = call_rt(f, s->x, s->y);
		flags_text(fetestexcept(FE_ALL_EXCEPT), rt_flags);
		feclearexcept(FE_ALL_EXCEPT);
		aor = call_aor(f, s->x, s->y);
		flags_text(fetestexcept(FE_ALL_EXCEPT), aor_flags);
		printf("special %s %a %a rt %a %s aor %a %s\n", s->name, s->x, s->y, rt, rt_flags, aor, aor_flags);
	}
}

/*
 * Records written on a target by tools/aor/arm-sample.c: per function a
 * 16-byte name, then samples records of (x, y, result) as doubles; one
 * library's results, measured to nearest
 */
static int run_records(const char *label, const char *path, uint64_t samples)
{
	FILE *in = fopen(path, "rb");
	char name[17] = { 0 };
	struct checker c;

	if (!in) {
		perror(path);
		return 1;
	}
	checker_init(&c);
	while (fread(name, 16, 1, in) == 1) {
		const struct function *f = find(name);
		struct tally t = { 0 };

		for (uint64_t k = 0; k < samples; ++k) {
			double record[3];

			if (fread(record, sizeof(record), 1, in) != 1) {
				fprintf(stderr, "%s: short file\n", path);
				return 1;
			}
			if (exact_value(&c, f, record[0], record[1]))
				measure(&c, f, record[2], MPFR_RNDN, &t, record[0], record[1]);
		}
		printf("records %s %s nearest max %.4f at %a %a wrong %" PRIu64 " of %" PRIu64 "\n", label, f->name, t.max_error, t.worst_x, t.worst_y, t.wrong, t.checked);
	}
	checker_clear(&c);
	fclose(in);
	return 0;
}

int main(int argc, char *argv[])
{
	static const int modes[] = { FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO };

	if (argc >= 3 && strcmp(argv[1], "exhaustive") == 0) {
		run(find(argv[2]), true, 0, FE_TONEAREST, "exhaustive");
	} else if (argc >= 4 && strcmp(argv[1], "sample") == 0) {
		run(find(argv[2]), false, strtoull(argv[3], nullptr, 0), FE_TONEAREST, "sample");
	} else if (argc >= 4 && strcmp(argv[1], "modes") == 0) {
		for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i)
			run(find(argv[2]), false, strtoull(argv[3], nullptr, 0), modes[i], "modes");
	} else if (argc >= 5 && strcmp(argv[1], "records") == 0) {
		return run_records(argv[2], argv[3], strtoull(argv[4], nullptr, 0));
	} else if (argc >= 2 && strcmp(argv[1], "special") == 0) {
		run_special();
	} else {
		fprintf(stderr, "usage: accuracy exhaustive <f> | sample <f> <n> | modes <f> <n> | special | records <label> <file> <samples>\n");
		return 2;
	}
	return 0;
}
