/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * softdp.h
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _SOFTDP_H
#define _SOFTDP_H

#include <stdint.h>

#include "internal/aeabi.h"

/* Shared by the test's sources */
void fail(const char *what, uint64_t a, uint64_t b);
uint64_t next(uint64_t *state);
uint64_t operand(uint64_t *state, int kind);

/*
 * The soft-dp layer checked case by case (PLAN D99, D102): its C helpers
 * and the models of its assembly against the hardware on x86_64
 * (direct.c), its assembly against the models on the cores that link it
 * (assembly.c), nothing elsewhere (layer_none.c). iterations random cases,
 * and every float_stride-th float through the conversion from float.
 */
void test_layer(unsigned long iterations, unsigned long float_stride);

/* The models of __aeabi_dadd and __aeabi_dmul (reference_add.c, reference_multiply.c) */
__LIBC_AEABI double reference_add(double a, double b);
__LIBC_AEABI double reference_multiply(double a, double b);

/* The model of the comparisons (reference_compare.c): the ordering of the doubles with these bits */
enum {
	REFERENCE_LESS,
	REFERENCE_EQUAL,
	REFERENCE_GREATER,
	REFERENCE_UNORDERED,
};

int reference_compare(uint64_t ua, uint64_t ub);

/* The models of the conversions written only in assembly (reference_convert.c), on bits */
uint64_t reference_f2d(uint32_t bits);
int32_t reference_d2iz(uint64_t bits);
uint32_t reference_d2uiz(uint64_t bits);
uint64_t reference_ul2d(uint64_t sign, uint64_t u);
uint64_t reference_l2d(int64_t a);

#endif /* _SOFTDP_H */
