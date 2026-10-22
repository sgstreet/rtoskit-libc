/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_f2d.S
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * (double)a, exact (C23 6.3.1.5p1, RTABI 4.1.2; PLAN D99). a is r0, the
 * result r1:r0. A normal float's exponent field moves 20 bits down with the
 * significand above it, and the bias grows by 1023 - 127; the low 3 bits of
 * the significand go to the low word. A subnormal float is normalised
 * first, and a NaN keeps its payload, quietened.
 */
	.syntax unified
	.thumb
	.section .text.__aeabi_f2d,"ax",%progbits
	.global __aeabi_f2d
	.type __aeabi_f2d, %function
__aeabi_f2d:
	/*
	 * r2 = the bits without the sign, shifted up one, C the sign; r1 =
	 * sign | the bits >> 3, with the bias grown; normal floats lie in
	 * [2^24, 0xff000000)
	 */
	lsls r2, r0, #1
	lsr r1, r2, #3
	rrx r1, r1
	add r1, r1, #0x38000000
	sub r3, r2, #0x01000000
	cmp r3, #0xfe000000
	bhs 1f
	lsls r0, r0, #29
	bx lr
1:
	and r3, r0, #0x80000000
	cmp r2, #0xff000000
	bhs 2f

	/* Zero: the sign alone */
	cmp r2, #0
	ittt eq
	moveq r1, r3
	moveq r0, #0
	bxeq lr

	/* Subnormal: shift the leading bit to bit 23, sh = clz(a) - 8 places, and the exponent field 897 - sh */
	clz ip, r2
	sub ip, ip, #7
	lsrs r2, r2, #1
	lsls r2, r2, ip
	rsb r1, ip, #896
	lsls r1, r1, #20
	add r1, r1, r2, lsr #3
	orrs r1, r1, r3
	lsls r0, r2, #29
	bx lr
2:
	/* Infinity, or a NaN (a nonzero significand), quietened */
	ubfx r1, r0, #0, #23
	cmp r1, #0
	lsr r1, r1, #3
	orr r1, r1, #0x7f000000
	orr r1, r1, #0x00f00000
	it ne
	orrne r1, r1, #0x00080000
	orrs r1, r1, r3
	lsls r0, r0, #29
	bx lr
	.size __aeabi_f2d, . - __aeabi_f2d
