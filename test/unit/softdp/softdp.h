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
 * The soft-dp layer checked case by case (PLAN D99): its C helpers and the
 * models of its assembly against the hardware on x86_64 (direct.c), its
 * assembly against the models on the cores that link it (assembly.c),
 * nothing elsewhere (layer_none.c)
 */
void test_layer(void);

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

#endif /* _SOFTDP_H */
