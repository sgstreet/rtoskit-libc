/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fpv5.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>

/*
 * The FPv5 instructions the math library will rely on (PLAN D18) give the
 * architected results under qemu-arm: VRINT in every rounding mode, VMAXNM
 * and VMINNM with a quiet NaN operand, a fused VFMA.F64 that rounds once,
 * VCVTA and VSQRT, in double and single precision. Inline assembly, so the
 * instructions run in debug builds too. Built only for fpv5-dp (subdir.mk).
 */

static int failures;

static void fail(int line, const char *what)
{
	char message[256];

	snprintf(message, sizeof(message), "fail: line %d: %s\n", line, what);
	fputs(message, stdout);
	++failures;
}

#define CHECK(condition) ((condition) ? (void)0 : fail(__LINE__, #condition))

/* Volatile so the operands are loaded at run time, never folded */
static volatile double two_and_half = 2.5;
static volatile double minus_one_and_half = -1.5;
static volatile double three = 3.0;
static volatile float two_and_half_f = 2.5f;
static volatile float minus_half_f = -0.5f;

#define RINT_F64(instruction, x)                                  \
	({                                                            \
		double result;                                            \
		__asm__(instruction " %P0, %P1" : "=w"(result) : "w"(x)); \
		result;                                                   \
	})

int main(void)
{
	double a = two_and_half;
	double b = minus_one_and_half;
	double nan = __builtin_nan("");
	double result;
	float f;
	int integer;

	/* C23 F.10.6: ties away (round), to even, down (floor), up (ceil), toward zero (trunc) */
	CHECK(RINT_F64("vrinta.f64", a) == 3.0);
	CHECK(RINT_F64("vrintn.f64", a) == 2.0);
	CHECK(RINT_F64("vrintm.f64", b) == -2.0);
	CHECK(RINT_F64("vrintp.f64", b) == -1.0);
	CHECK(RINT_F64("vrintz.f64", b) == -1.0);
	/* vrintr and vrintx use the FPSCR mode, round to nearest even by default */
	CHECK(RINT_F64("vrintr.f64", a) == 2.0);
	CHECK(RINT_F64("vrintx.f64", b) == -2.0);

	/* IEEE 754-2008 maxNum/minNum: a quiet NaN operand is ignored */
	__asm__("vmaxnm.f64 %P0, %P1, %P2" : "=w"(result) : "w"(a), "w"(nan));
	CHECK(result == 2.5);
	__asm__("vminnm.f64 %P0, %P1, %P2" : "=w"(result) : "w"(nan), "w"(b));
	CHECK(result == -1.5);

	/* (1 + 2^-30)(1 - 2^-30) - 1 is -2^-60 only when the product is not rounded first */
	double x = 1.0 + 0x1p-30 * (a - 1.5);
	double y = 1.0 - 0x1p-30 * (a - 1.5);
	double accumulator = -1.0;

	__asm__("vfma.f64 %P0, %P1, %P2" : "+w"(accumulator) : "w"(x), "w"(y));
	CHECK(accumulator == -0x1p-60);

	__asm__("vsqrt.f64 %P0, %P1" : "=w"(result) : "w"(three * three));
	CHECK(result == 3.0);

	__asm__("vcvta.s32.f64 %0, %P1" : "=t"(f) : "w"(a));
	__builtin_memcpy(&integer, &f, sizeof(integer));
	CHECK(integer == 3);

	/* Single precision */
	__asm__("vrinta.f32 %0, %1" : "=t"(f) : "t"(two_and_half_f));
	CHECK(f == 3.0f);
	__asm__("vmaxnm.f32 %0, %1, %2" : "=t"(f) : "t"(two_and_half_f), "t"(minus_half_f));
	CHECK(f == 2.5f);

	if (failures == 0)
		puts("pass: fpv5");

	return failures == 0 ? 0 : 1;
}
