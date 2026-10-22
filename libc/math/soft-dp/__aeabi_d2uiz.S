/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_d2uiz.S
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * (unsigned int)a, truncated toward zero (C23 6.3.1.4p1, RTABI 4.1.2;
 * PLAN D99). a is r1:r0. For an unbiased exponent t in [0, 31] the result
 * is the top 32 bits of the significand shifted right 31 - t places. Below
 * that the result is 0; above it, and for a negative a, the behaviour is
 * undefined: the result saturates, and a negative value and NaN give 0, as
 * the FPU's own conversion does. Weak, as __aeabi_dadd.
 */
	.syntax unified
	.thumb
	.section .text.__aeabi_d2uiz,"ax",%progbits
	.weak __aeabi_d2uiz
	.type __aeabi_d2uiz, %function
__aeabi_d2uiz:
	ubfx r2, r1, #20, #11
	subw r2, r2, #1023
	cmp r1, #0
	blt 2f
	cmp r2, #32
	bhs 1f
	rsb r2, r2, #31
	lsls r3, r1, #11
	orr r3, r3, r0, lsr #21
	orr r3, r3, #0x80000000
	lsr r0, r3, r2
	bx lr
1:
	/* t < 0: a < 1 */
	cmp r2, #0
	blt 2f
	/* NaN: the bits without the sign above infinity's, the low word folded in */
	lsls r3, r1, #1
	cmp r0, #0
	it ne
	orrne r3, r3, #1
	cmp r3, #0xffe00000
	bhi 2f
	mov r0, #0xffffffff
	bx lr
2:
	movs r0, #0
	bx lr
	.size __aeabi_d2uiz, . - __aeabi_d2uiz
