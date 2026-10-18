/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tls.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_TLS_H
#define _INTERNAL_TLS_H

#include <stddef.h>
#include <stdint.h>
#include <rtos/rtos-port.h>

/*
 * Helpers shared by the arch TLS layouts. The formulas follow the ELF TLS
 * ABI; see docs/research/tls-and-startup.md.
 */

static inline size_t __libc_tls_segment_align(const struct __libc_tls_image *image)
{
	return image->align > 1 ? image->align : 1;
}

static inline uintptr_t __libc_tls_align_up(uintptr_t value, size_t align)
{
	return (value + align - 1) & ~(uintptr_t)(align - 1);
}

/*
 * Variant II (TLS below the thread pointer): the distance from the thread
 * pointer down to the start of the TLS data. It is congruent to minus the
 * image address modulo the segment alignment, which is what the linker
 * assumes when it resolves local-exec offsets.
 */
static inline size_t __libc_tls_variant2_offset(const struct __libc_tls_image *image)
{
	size_t align = __libc_tls_segment_align(image);
	return image->size + ((-(uintptr_t)image->init - image->size) & (align - 1));
}

/*
 * Variant I (TLS above the thread pointer): the distance from the thread
 * pointer up to the start of the TLS data, given the reserved control block
 * size at the thread pointer.
 */
static inline size_t __libc_tls_variant1_offset(const struct __libc_tls_image *image, size_t tcb_size)
{
	size_t align = __libc_tls_segment_align(image);
	return tcb_size + (((uintptr_t)image->init - tcb_size) & (align - 1));
}

/* Copy the initialization image and zero fill the remainder */
static inline void __libc_tls_copy_image(unsigned char *data, const struct __libc_tls_image *image)
{
	const unsigned char *init = image->init;
	for (size_t i = 0; i < image->init_size; ++i)
		data[i] = init[i];
	for (size_t i = image->init_size; i < image->size; ++i)
		data[i] = 0;
}

#endif
