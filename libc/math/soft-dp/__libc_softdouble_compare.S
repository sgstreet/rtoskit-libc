/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_softdouble_compare.S
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * The ordering of a (r1:r0) and b (r3:r2) in the flags, as CMP would set
 * them for two ordered integers (IEEE 754-2019 5.11; PLAN D101): Z set for
 * equal, C clear for less; unordered (a NaN on either side) is C set and Z
 * clear, which no condition the callers test reads as less or equal. The
 * zeros are equal. Otherwise, same signs compare as their bits, as
 * unsigned 64-bit integers, reversed for two negatives (two zeros of one
 * sign come out equal there); opposite signs order by the sign alone. The
 * common path tests each operand for an exponent field of 2047 with one
 * compare, and only then for a NaN. Only ip and the flags change, so the
 * callers keep their operands.
 */
	.syntax unified
	.thumb
	.section .text.__libc_softdouble_compare,"ax",%progbits
	.global __libc_softdouble_compare
	.hidden __libc_softdouble_compare
	.type __libc_softdouble_compare, %function
__libc_softdouble_compare:
	/* An exponent field of 2047 (infinity or NaN) on either side: tested closely below */
	lsls ip, r1, #1
	cmn ip, #0x00200000
	bcs 3f
	lsls ip, r3, #1
	cmn ip, #0x00200000
	bcs 3f
1:
	/* Opposite signs: the zeros are equal; otherwise a is less exactly when it is the negative one */
	teq r1, r3
	bmi 5f
	cmp r1, #0
	blt 2f
	cmp r1, r3
	it eq
	cmpeq r0, r2
	bx lr
2:
	/* Two negatives: the larger bits are the smaller number */
	cmp r3, r1
	it eq
	cmpeq r2, r0
	bx lr
3:
	/* A NaN, the bits without the sign above infinity's, is unordered; two infinities order as above */
	lsls ip, r1, #1
	cmp ip, #0xffe00000
	it eq
	cmpeq r0, #0
	bhi 4f
	lsls ip, r3, #1
	cmp ip, #0xffe00000
	it eq
	cmpeq r2, #0
	bls 1b
4:
	/* Unordered: C set, Z clear */
	movs ip, #1
	cmp ip, #0
	bx lr
5:
	orr ip, r0, r1, lsl #1
	orr ip, ip, r2
	orrs ip, ip, r3, lsl #1
	beq 6f
	cmp r3, r1
	bx lr
6:
	cmp ip, ip
	bx lr
	.size __libc_softdouble_compare, . - __libc_softdouble_compare
