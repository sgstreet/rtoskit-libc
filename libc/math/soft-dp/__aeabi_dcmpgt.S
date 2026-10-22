/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_dcmpgt.S
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * a > b, 1 or 0 (IEEE 754-2019 5.11, RTABI 4.1.2; PLAN D101): false when
 * either operand is a NaN. The operands are exchanged first,
 * so that the ordering's "less" tests b < a.
 */
	.syntax unified
	.thumb
	.section .text.__aeabi_dcmpgt,"ax",%progbits
	.global __aeabi_dcmpgt
	.type __aeabi_dcmpgt, %function
__aeabi_dcmpgt:
	push {r4, lr}
	mov ip, r0
	mov r0, r2
	mov r2, ip
	mov ip, r1
	mov r1, r3
	mov r3, ip
	bl __libc_softdouble_compare
	ite lo
	movlo r0, #1
	movhs r0, #0
	pop {r4, pc}
	.size __aeabi_dcmpgt, . - __aeabi_dcmpgt
