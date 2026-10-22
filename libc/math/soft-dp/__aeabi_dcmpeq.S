/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_dcmpeq.S
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * a == b, 1 or 0 (IEEE 754-2019 5.11, RTABI 4.1.2; PLAN D101): false when
 * either operand is a NaN.
 */
	.syntax unified
	.thumb
	.section .text.__aeabi_dcmpeq,"ax",%progbits
	.global __aeabi_dcmpeq
	.type __aeabi_dcmpeq, %function
__aeabi_dcmpeq:
	push {r4, lr}
	bl __libc_softdouble_compare
	ite eq
	moveq r0, #1
	movne r0, #0
	pop {r4, pc}
	.size __aeabi_dcmpeq, . - __aeabi_dcmpeq
