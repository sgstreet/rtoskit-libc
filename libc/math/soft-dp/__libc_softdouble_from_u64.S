/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_softdouble_from_u64.S
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * The double nearest the 64-bit unsigned integer r1:r0, with the sign r2
 * (0 or 2^31) above it (C23 6.3.1.4p2; PLAN D99), for __aeabi_ul2d and
 * __aeabi_l2d. Below 2^32 the value is exact: the word normalised by its
 * leading zeros k, and the exponent field 1054 - k. Above, the value is
 * normalised to bit 63: its top 53 bits are the significand, bit 10 is
 * half an ulp, bits 9 to 0 are sticky, and a tie clears the last bit, for
 * the even neighbour; the exponent field is 1086 - k.
 */
	.syntax unified
	.thumb
	.section .text.__libc_softdouble_from_u64,"ax",%progbits
	.global __libc_softdouble_from_u64
	.hidden __libc_softdouble_from_u64
	.type __libc_softdouble_from_u64, %function
__libc_softdouble_from_u64:
	cbz r1, 2f
	clz r3, r1
	rsb ip, r3, #32
	lsls r1, r1, r3
	lsr ip, r0, ip
	orrs r1, r1, ip
	lsls r0, r0, r3

	/* ip = bits 10 to 0 at the top; r1:r0 = the top 53 bits; r1 += sign | (1085 - k) 2^20 */
	lsls ip, r0, #21
	lsrs r0, r0, #11
	orr r0, r0, r1, lsl #21
	subw r3, r3, #1085
	negs r3, r3
	lsls r3, r3, #20
	add r1, r3, r1, lsr #11
	orrs r1, r1, r2

	lsls r3, ip, #1
	bcc 1f
	adds r0, r0, #1
	adc r1, r1, #0
	cmp r3, #0
	it eq
	biceq r0, r0, #1
1:
	bx lr
2:
	/* Below 2^32: exact, and zero is +0 */
	cbz r0, 3f
	clz r3, r0
	lsls r0, r0, r3
	subw r3, r3, #1053
	negs r3, r3
	lsls r3, r3, #20
	add r1, r3, r0, lsr #11
	orrs r1, r1, r2
	lsls r0, r0, #21
3:
	bx lr
	.size __libc_softdouble_from_u64, . - __libc_softdouble_from_u64
