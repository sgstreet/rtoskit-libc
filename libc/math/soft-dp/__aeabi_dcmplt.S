/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_dcmplt.S
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * a < b, 1 or 0 (IEEE 754-2019 5.11, RTABI 4.1.2; PLAN D101): false when
 * either operand is a NaN.
 */
	.syntax unified
	.thumb
	.section .text.__aeabi_dcmplt,"ax",%progbits
	.global __aeabi_dcmplt
	.type __aeabi_dcmplt, %function
__aeabi_dcmplt:
	push {r4, lr}
	bl __libc_softdouble_compare
	ite lo
	movlo r0, #1
	movhs r0, #0
	pop {r4, pc}
	.size __aeabi_dcmplt, . - __aeabi_dcmplt
