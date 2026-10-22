/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_dsub.S
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * a - b is a + (-b) (IEEE 754-2019 5.4.1), with the same result and sign
 * of zero (PLAN D55, D99): b's sign bit flips in r3, which is not a
 * floating-point operation, and the addition does the rest.
 */
	.syntax unified
	.thumb
	.section .text.__aeabi_dsub,"ax",%progbits
	.global __aeabi_dsub
	.type __aeabi_dsub, %function
__aeabi_dsub:
	eor r3, r3, #0x80000000
	b __aeabi_dadd
	.size __aeabi_dsub, . - __aeabi_dsub
