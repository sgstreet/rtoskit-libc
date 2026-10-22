/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_d2f.S
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * (float)a, rounded to nearest (C23 6.3.1.5p2, RTABI 4.1.2; PLAN D99). a
 * is r1:r0, the result r0. For a result in float's normal range (an
 * exponent field from 897 to 1150) the top 24 bits of the significand are
 * rounded on the 29 below them: bit 28 of the low word is half an ulp, the
 * rest sticky, and a tie clears the last bit, for the even neighbour. A
 * carry moves into the exponent, and from the largest finite float to
 * infinity. Anything else goes to __libc_softdouble_d2f, the general C
 * version. Weak, as __aeabi_dadd.
 */
	.syntax unified
	.thumb
	.section .text.__aeabi_d2f,"ax",%progbits
	.weak __aeabi_d2f
	.type __aeabi_d2f, %function
__aeabi_d2f:
	ubfx r2, r1, #20, #11
	subw r2, r2, #897
	cmp r2, #253
	bhi 2f

	/*
	 * sign | the low 8 bits of the field | the significand's top 23 bits:
	 * the field less 896 is those 8 bits with the top one flipped. C is
	 * then bit 28, half an ulp, and ip the sticky bits below it
	 */
	lsls r3, r1, #1
	lsl r2, r1, #4
	rrx r2, r2
	eor r2, r2, #0x40000000
	lsls ip, r0, #4
	orr r0, r2, r0, lsr #29

	/* Round to nearest: a tie (no sticky bit) clears the last bit, for the even neighbour */
	bcc 1f
	adds r0, r0, #1
	cmp ip, #0
	it eq
	biceq r0, r0, #1
1:
	bx lr
2:
	b __libc_softdouble_d2f
	.size __aeabi_d2f, . - __aeabi_d2f
