/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tls-arch.h
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_TLS_ARCH_H
#define _INTERNAL_TLS_ARCH_H

#include <stdint.h>

/*
 * Arm uses TLS variant I: the thread pointer (__aeabi_read_tp) addresses a
 * two-word control block reserved by the ABI, and the TLS data follows above
 * it. The linker resolves local-exec offsets assuming exactly this gap.
 * libc keeps nothing in the block.
 */
struct __libc_tcb {
	uintptr_t reserved[2];
};

/* Thread pointer alignment: the control block holds words, and 8 keeps AAPCS doubleword alignment */
#define __LIBC_TCB_ALIGN 8

#endif /* _INTERNAL_TLS_ARCH_H */
