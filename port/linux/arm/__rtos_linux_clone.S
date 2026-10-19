/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_linux_clone.S
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * long __rtos_linux_clone(flags r0, stack r1, parent_tid r2, child_tid r3,
 *                         tls [sp], fn [sp, #4], arg [sp, #8])
 *
 * The kernel takes flags, stack, parent_tid, tls, child_tid (r0-r4) and the
 * call number in r7. fn and arg travel on the child's stack (8-byte
 * aligned). Thumb-1 only, so it assembles for armv6-m; svc is emitted as
 * .inst.n because -march=armv6-m rejects it (docs/research/arm-bring-up.md).
 */
	.syntax unified
	.thumb
	.text
	.global __rtos_linux_clone
	.type __rtos_linux_clone, %function
__rtos_linux_clone:
	push {r4, r5, r6, r7}
	mov r4, r3
	ldr r3, [sp, #16]
	ldr r5, [sp, #20]
	ldr r6, [sp, #24]
	lsrs r1, r1, #3
	lsls r1, r1, #3
	subs r1, r1, #8
	str r5, [r1]
	str r6, [r1, #4]
	movs r7, #120
	.inst.n 0xdf00
	cmp r0, #0
	beq 1f
	pop {r4, r5, r6, r7}
	bx lr
1:
	pop {r0, r1}
	mov r2, r0
	mov r0, r1
	blx r2
	movs r7, #1
	.inst.n 0xdf00
	udf #0
	.size __rtos_linux_clone, . - __rtos_linux_clone

	.section .note.GNU-stack, "", %progbits
