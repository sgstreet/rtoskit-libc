/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * bench_cycles.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <time.h>

#include "bench.h"

/*
 * qemu-user has no cycle counter to offer an M-profile program, and the
 * Linux test port cannot reach the DWT, so time mode on Arm reports
 * nanoseconds from the port's clock. The figures that count on Arm are the
 * plugin's instruction counts (PLAN Q5).
 */
unsigned long long bench_cycles(void)
{
	struct timespec now;

	timespec_get(&now, TIME_MONOTONIC);
	return (unsigned long long)now.tv_sec * 1000000000ULL + (unsigned long long)now.tv_nsec;
}
