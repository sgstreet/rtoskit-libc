/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_tlsf_get.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <rtos/rtos-port.h>

#include "internal/malloc.h"
#include "tlsf.h"

/* The heap, created at the first allocation; protected by __rtos_malloc_lock */
static struct __libc_tlsf *heap;
static bool initialised;

static uintptr_t align_up(uintptr_t x)
{
	return (x + __LIBC_TLSF_ALIGN - 1) & ~(uintptr_t)(__LIBC_TLSF_ALIGN - 1);
}

/* One free block spanning the region and a zero-stride used sentinel after it */
static void add_region(struct __libc_tlsf *control, uintptr_t base, uintptr_t end)
{
	uintptr_t first;
	uintptr_t last;
	struct __libc_tlsf_region *region;

	/* The first header sits inside the region, and the sentinel's header too */
	if (end - base < 2 * __LIBC_TLSF_MIN_STRIDE)
		return;
	first = align_up(base + __LIBC_TLSF_WORD);
	last = end & ~(uintptr_t)(__LIBC_TLSF_ALIGN - 1);
	if (last <= first || last - first < __LIBC_TLSF_MIN_STRIDE)
		return;

	region = &control->regions[control->region_count++];
	region->first = (void *)first;
	region->end = (void *)last;

	*__libc_tlsf_header(region->first) = (last - first) | __LIBC_TLSF_FREE;
	*__libc_tlsf_back_pointer(region->end) = region->first;
	*__libc_tlsf_header(region->end) = 0 | __LIBC_TLSF_PREV_FREE;
	__libc_tlsf_insert(control, region->first);
}

static struct __libc_tlsf *create(void)
{
	void *bases[__LIBC_TLSF_MAX_REGIONS];
	size_t sizes[__LIBC_TLSF_MAX_REGIONS];
	unsigned int count = 0;
	unsigned int largest = 0;
	unsigned int fl;
	unsigned int sl;
	size_t control_size;
	uintptr_t control_base;
	struct __libc_tlsf *control;

	while (count < __LIBC_TLSF_MAX_REGIONS && __rtos_heap_region(count, &bases[count], &sizes[count])) {
		if (sizes[count] > sizes[largest])
			largest = count;
		++count;
	}
	if (count == 0)
		return nullptr;

	/* Enough first-level lists for the largest block the largest region can hold */
	__libc_tlsf_mapping(sizes[largest] < __LIBC_TLSF_SMALL ? __LIBC_TLSF_SMALL : sizes[largest], &fl, &sl);
	control_size = align_up(offsetof(struct __libc_tlsf, lists) + (fl + 1) * sizeof(((struct __libc_tlsf *)nullptr)->lists[0]));
	control_base = align_up((uintptr_t)bases[largest]);
	if (control_base + control_size + 2 * __LIBC_TLSF_MIN_STRIDE > (uintptr_t)bases[largest] + sizes[largest])
		return nullptr;

	control = (struct __libc_tlsf *)control_base;
	memset(control, 0, control_size);
	control->fl_count = fl + 1;

	for (unsigned int i = 0; i < count; ++i) {
		uintptr_t base = (uintptr_t)bases[i];
		uintptr_t end = base + sizes[i];

		if (i == largest)
			base = control_base + control_size;
		add_region(control, base, end);
	}

	return control->region_count > 0 ? control : nullptr;
}

struct __libc_tlsf *__libc_tlsf_get(void)
{
	if (!initialised) {
		heap = create();
		initialised = true;
	}

	return heap;
}
