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

#include "bench.h"

/*
 * The time stamp counter. lfence keeps the measured instructions from
 * starting before, or retiring after, the read (Intel SDM vol. 2, RDTSC).
 */
unsigned long long bench_cycles(void)
{
	unsigned int low;
	unsigned int high;

	__asm__ volatile("lfence\n\trdtsc\n\tlfence" : "=a"(low), "=d"(high) : : "memory");
	return ((unsigned long long)high << 32) | low;
}
