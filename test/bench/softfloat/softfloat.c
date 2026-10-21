/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * softfloat.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "bench.h"

/*
 * The floating-point operations a compiler turns into run-time library calls
 * where the core has no FPU for them (P4.03, PLAN D92): float and double
 * add, subtract, multiply, divide and compare, the conversions between them,
 * and to and from 32- and 64-bit integers. Without an FPU they are the
 * __aeabi_* helpers (ours, libgcc's, or Arm Optimized Routines' in the phase
 * 4 comparison); with one, most are an instruction, and the 64-bit integer
 * conversions remain calls on every core. Each case applies its operation to
 * 16 operand pairs; results are folded by their bits, with integer
 * operations only, so the folding adds no floating-point call. Operations
 * are the floating-point operations.
 */

#define OPERANDS 16

struct operands {
	float fa[OPERANDS];
	float fb[OPERANDS];
	double da[OPERANDS];
	double db[OPERANDS];
	int32_t ia[OPERANDS];
	int64_t la[OPERANDS];
};

static struct operands operands;

static unsigned long float_bits(float x)
{
	union {
		float f;
		uint32_t u;
	} v = { .f = x };

	return v.u;
}

static unsigned long double_bits(double x)
{
	union {
		double d;
		uint64_t u;
	} v = { .d = x };

	return (unsigned long)(v.u ^ v.u >> 32);
}

static unsigned long integer_bits(int64_t x)
{
	return (unsigned long)((uint64_t)x ^ (uint64_t)x >> 32);
}

#define BINARY_CASE(name, a, b, op, fold)                           \
	static void run_##name(void *context, unsigned long iterations) \
	{                                                               \
		const struct operands *o = context;                         \
                                                                    \
		for (unsigned long i = 0; i < iterations; ++i) {            \
			unsigned long bits = 0;                                 \
                                                                    \
			bench_clobber(o);                                       \
			for (size_t k = 0; k < OPERANDS; ++k)                   \
				bits ^= fold(o->a[k] op o->b[k]);                   \
			bench_sink = bits;                                      \
		}                                                           \
	}

#define COMPARE_CASE(name, a, b)                                    \
	static void run_##name(void *context, unsigned long iterations) \
	{                                                               \
		const struct operands *o = context;                         \
                                                                    \
		for (unsigned long i = 0; i < iterations; ++i) {            \
			unsigned long count = 0;                                \
                                                                    \
			bench_clobber(o);                                       \
			for (size_t k = 0; k < OPERANDS; ++k)                   \
				count += o->a[k] < o->b[k];                         \
			bench_sink = count;                                     \
		}                                                           \
	}

#define CONVERT_CASE(name, from, type, fold)                        \
	static void run_##name(void *context, unsigned long iterations) \
	{                                                               \
		const struct operands *o = context;                         \
                                                                    \
		for (unsigned long i = 0; i < iterations; ++i) {            \
			unsigned long bits = 0;                                 \
                                                                    \
			bench_clobber(o);                                       \
			for (size_t k = 0; k < OPERANDS; ++k)                   \
				bits ^= fold((type)o->from[k]);                     \
			bench_sink = bits;                                      \
		}                                                           \
	}

BINARY_CASE(fadd, fa, fb, +, float_bits)
BINARY_CASE(fsub, fa, fb, -, float_bits)
BINARY_CASE(fmul, fa, fb, *, float_bits)
BINARY_CASE(fdiv, fa, fb, /, float_bits)
COMPARE_CASE(fcmp, fa, fb)
BINARY_CASE(dadd, da, db, +, double_bits)
BINARY_CASE(dsub, da, db, -, double_bits)
BINARY_CASE(dmul, da, db, *, double_bits)
BINARY_CASE(ddiv, da, db, /, double_bits)
COMPARE_CASE(dcmp, da, db)
CONVERT_CASE(f2d, fa, double, double_bits)
CONVERT_CASE(d2f, da, float, float_bits)
CONVERT_CASE(f2i, fa, int32_t, integer_bits)
CONVERT_CASE(i2f, ia, float, float_bits)
CONVERT_CASE(d2i, da, int32_t, integer_bits)
CONVERT_CASE(i2d, ia, double, double_bits)
CONVERT_CASE(f2l, fa, int64_t, integer_bits)
CONVERT_CASE(l2f, la, float, float_bits)
CONVERT_CASE(d2l, da, int64_t, integer_bits)
CONVERT_CASE(l2d, la, double, double_bits)

struct softfloat_case {
	const char *name;
	void (*body)(void *context, unsigned long iterations);
};

static const struct softfloat_case cases[] = {
	{ "fadd", run_fadd },
	{ "fsub", run_fsub },
	{ "fmul", run_fmul },
	{ "fdiv", run_fdiv },
	{ "fcmp", run_fcmp },
	{ "dadd", run_dadd },
	{ "dsub", run_dsub },
	{ "dmul", run_dmul },
	{ "ddiv", run_ddiv },
	{ "dcmp", run_dcmp },
	{ "f2d", run_f2d },
	{ "d2f", run_d2f },
	{ "f2i", run_f2i },
	{ "i2f", run_i2f },
	{ "d2i", run_d2i },
	{ "i2d", run_i2d },
	{ "f2l", run_f2l },
	{ "l2f", run_l2f },
	{ "d2l", run_d2l },
	{ "l2d", run_l2d },
};

/*
 * Operands with full random significands, both signs and exponents over a
 * few decades (from a fixed linear congruential sequence, so every run and
 * every library sees the same ones); none is zero, so the divisions are
 * ordinary ones, and the integer conversions stay in range.
 */
static uint32_t next(uint32_t *state)
{
	*state = *state * 1664525u + 1013904223u;
	return *state;
}

static double operand(uint32_t *state)
{
	double significand = 1.0 + (double)(next(state) >> 1) / 2147483648.0;
	int exponent = (int)(next(state) % 21) - 10;
	double value = significand;

	for (; exponent > 0; --exponent)
		value *= 2.0;
	for (; exponent < 0; ++exponent)
		value *= 0.5;
	return (next(state) & 1) ? -value : value;
}

static void prepare(struct operands *o)
{
	uint32_t state = 12345;

	for (size_t k = 0; k < OPERANDS; ++k) {
		o->fa[k] = (float)operand(&state);
		o->fb[k] = (float)operand(&state);
		o->da[k] = operand(&state) * 1e5 + operand(&state);
		o->db[k] = operand(&state);
		o->ia[k] = (int32_t)next(&state);
		o->la[k] = (int64_t)((uint64_t)next(&state) << 31 ^ next(&state));
	}
}

int main(int argc, char *argv[])
{
	bench_init(argc, argv);
	prepare(&operands);
	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
		bench_run(cases[i].name, OPERANDS, cases[i].body, &operands);
	return bench_finish();
}
