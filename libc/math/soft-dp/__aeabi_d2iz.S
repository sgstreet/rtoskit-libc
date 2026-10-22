/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_d2iz.S
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * (int)a, truncated toward zero (C23 6.3.1.4p1, RTABI 4.1.2; PLAN D99). a
 * is r1:r0. For an unbiased exponent t in [0, 30] the result is the top 32
 * bits of the significand shifted right 31 - t places. Below that the
 * result is 0; above it the behaviour is undefined, and the result
 * saturates and NaN gives 0, as the FPU's own conversion does.
 */
	.syntax unified
	.thumb
	.section .text.__aeabi_d2iz,"ax",%progbits
	.global __aeabi_d2iz
	.type __aeabi_d2iz, %function
__aeabi_d2iz:
	ubfx r2, r1, #20, #11
	subw r2, r2, #1023
	cmp r2, #31
	bhs 1f
	rsb r2, r2, #31
	lsls r3, r1, #11
	orr r3, r3, r0, lsr #21
	orr r3, r3, #0x80000000
	lsr r0, r3, r2
	cmp r1, #0
	it lt
	neglt r0, r0
	bx lr
1:
	/* t < 0: |a| < 1 */
	cmp r2, #0
	itt lt
	movlt r0, #0
	bxlt lr
	/* NaN: the bits without the sign above infinity's, the low word folded in */
	lsls r3, r1, #1
	cmp r0, #0
	it ne
	orrne r3, r3, #1
	cmp r3, #0xffe00000
	itt hi
	movhi r0, #0
	bxhi lr
	mvn r0, #0x80000000
	eor r0, r0, r1, asr #31
	bx lr
	.size __aeabi_d2iz, . - __aeabi_d2iz
