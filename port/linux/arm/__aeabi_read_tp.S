/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_read_tp.S
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * void *__aeabi_read_tp(void) (Arm RTABI 5.3, PLAN D10): return the thread
 * pointer in r0 and change no other register. M-profile code calls this for
 * every thread_local access. On Linux the kernel keeps the thread pointer in
 * TPIDRURO, set by the set_tls call; read it with
 * "mrc p15, 0, r0, c13, c0, 3", emitted as raw bits because the M-profile
 * architectures have no coprocessor 15 and the assembler rejects it. That
 * makes this port need an A-profile CPU model under qemu-arm (-cpu max).
 */
	.syntax unified
	.thumb
	.text
	.global __aeabi_read_tp
	.type __aeabi_read_tp, %function
__aeabi_read_tp:
	.inst.w 0xee1d0f70
	bx lr
	.size __aeabi_read_tp, . - __aeabi_read_tp

	.section .note.GNU-stack, "", %progbits
