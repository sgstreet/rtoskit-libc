/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * softfp.h
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _SOFTFP_H
#define _SOFTFP_H

#include <stdint.h>

/* Shared by the test's sources */
void fail(const char *what, uint64_t value);
uint64_t next(uint64_t *state);
uint32_t bits_of_float(float x);

/* The single-precision helpers against double, where they are soft-float calls (float_operations.c) */
void test_float_operations(void);

#endif /* _SOFTFP_H */
