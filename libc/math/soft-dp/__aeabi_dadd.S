/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_dadd.S
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * a + b (IEEE 754-2019 5.4.1, RTABI 4.1.2) for the Thumb-2 cores with a
 * single-precision FPU, round to nearest, no exception flags (PLAN D44,
 * D99, docs/research/softdouble.md).
 *
 * a is r1:r0 and b r3:r2 (base calling convention), exchanged first so
 * that |a| >= |b| as bits without the signs. Then only a can be an
 * infinity or a NaN (field 2047), and only b a zero or a subnormal (field
 * 0) unless both are. b's significand r7:r8 is shifted right by the
 * exponent difference d into three words r7:r8:ip, ip holding the bits
 * shifted out: all of them for d < 32, and above that the top 32 with a
 * sticky bit for anything lower, which keeps the two bits below the result
 * exact through the one-place normalisation a subtraction may need. d is
 * capped at 63, which leaves b below a quarter of a's last place, a sticky
 * bit. A subnormal b has no hidden bit and the exponent of field 1.
 *
 * Same signs add, and a carry into bit 53 shifts the sum right one place.
 * Opposite signs subtract (the borrow running through ip); a difference
 * below 2^52 shifts left one place, and one below 2^51 (a cancellation of
 * two or more bits, so exponents at most one apart) is exact and is
 * normalised by its leading zeros. Neither shift goes below the exponent
 * of field 1: a result there is subnormal, and exact. The rounding is to
 * nearest on ip: its top bit is half an ulp, the rest sticky, and a tie
 * clears the last bit, for the even neighbour; a carry moves into the
 * exponent, and to infinity at the top.
 *
 * Zeros: x + 0 is x, two zeros give -0 only if both are, and an exact zero
 * difference is +0 (6.3). Two subnormals add or subtract exactly as
 * integers. Infinity minus infinity is the default NaN (7.2); of two NaNs
 * the one with the larger payload is returned, quietened (6.2.3).
 */
	.syntax unified
	.thumb
	.section .text.__aeabi_dadd,"ax",%progbits
	.global __aeabi_dadd
	.type __aeabi_dadd, %function
__aeabi_dadd:
	push {r4-r8, lr}

	/* |a| >= |b|: compare the bits without the signs, and exchange the operands if not */
	lsls r6, r1, #1
	lsls r7, r3, #1
	cmp r0, r2
	sbcs ip, r6, r7
	bcs 1f
	mov ip, r0
	mov r0, r2
	mov r2, ip
	mov ip, r1
	mov r1, r3
	mov r3, ip
1:
	/* a's field 2047 (infinity, NaN) and b's 0 (zero, subnormal) are special */
	ubfx r4, r1, #20, #11
	ubfx r5, r3, #20, #11
	add ip, r4, #1
	tst ip, #0x800
	bne .Linfnan
	cmp r5, #0
	beq .Lsmall
	ubfx r7, r3, #0, #20
	orr r7, r7, #0x00100000
	subs r5, r4, r5

.Lalign:
	/* d = r5, at most 63; b's significand r7:r2 shifted right d places into r7:r8:ip */
	cmp r5, #63
	it hi
	movhi r5, #63
	mov r8, r2
	cmp r5, #32
	bhs 2f
	rsb lr, r5, #32
	lsl ip, r8, lr
	lsr r8, r8, r5
	lsl lr, r7, lr
	orr r8, r8, lr
	lsr r7, r7, r5
	b 3f
2:
	/* From 32 places, s = d - 32: ip = the top 32 bits shifted out, with a sticky bit for the rest */
	subs r5, r5, #32
	rsb lr, r5, #32
	lsl ip, r7, lr
	lsr r6, r8, r5
	orr ip, ip, r6
	lsls r6, r8, lr
	it ne
	orrne ip, ip, #1
	lsr r8, r7, r5
	movs r7, #0
3:
	/* a's significand with its hidden bit in r6:r0; opposite signs subtract */
	ubfx r6, r1, #0, #20
	orr r6, r6, #0x00100000
	teq r1, r3
	bmi .Lsubtract

	/* r6:lr = the sum, below 2^54; from 2^53 shift right one place, keeping ip's sticky bit */
	adds lr, r0, r8
	adc r6, r6, r7
	tst r6, #0x00200000
	beq .Lpack
	and r5, ip, #1
	orr ip, r5, ip, lsr #1
	lsrs r6, r6, #1
	rrxs lr, lr
	it cs
	orrcs ip, ip, #0x80000000
	adds r4, r4, #1
	add r5, r4, #1
	tst r5, #0x800
	beq .Lpack

	/* Overflow: infinity, with a's sign */
	and r1, r1, #0x80000000
	orr r1, r1, #0x7f000000
	orr r1, r1, #0x00f00000
	movs r0, #0
	pop {r4-r8, pc}

.Lsubtract:
	/* r6:lr:ip = a's significand less b's, at least 0 since |a| >= |b| */
	negs ip, ip
	sbcs lr, r0, r8
	sbc r6, r6, r7
	tst r6, #0x00100000
	bne .Lpack
	tst r6, #0x00080000
	beq .Lcancel
	cmp r4, #1
	beq .Lpack
	adds ip, ip, ip
	adcs lr, lr, lr
	adc r6, r6, r6
	subs r4, r4, #1

.Lpack:
	/* a's sign | (exponent - 1) 2^20 + the significand, whose hidden bit adds the one back */
	subs r4, r4, #1
	and r5, r1, #0x80000000
	add r5, r5, r4, lsl #20
	add r5, r5, r6

	/* Round to nearest: ip's top bit is half an ulp; a tie (the rest zero) clears the last bit */
	lsls r6, ip, #1
	bcc 4f
	adds lr, lr, #1
	adc r5, r5, #0
	cmp r6, #0
	it eq
	biceq lr, lr, #1
4:
	mov r0, lr
	mov r1, r5
	pop {r4-r8, pc}

.Lcancel:
	/*
	 * Two or more bits cancelled: the difference is exact, with at most
	 * ip's top bit below the low word. Normalise it by k places, k = clz -
	 * 11 from the high word or clz + 21 from the low one (53 for ip's bit
	 * alone), but no further than the exponent of field 1; an exact zero
	 * is +0 in round to nearest.
	 */
	orrs r5, r6, lr
	orrs r5, r5, ip
	beq .Lzero
	cbz r6, 5f
	clz r5, r6
	subs r5, r5, #11
	b 6f
5:
	clz r5, lr
	adds r5, r5, #21
6:
	cmp r5, r4
	it hs
	subhs r5, r4, #1
	subs r4, r4, r5
	cmp r5, #32
	bhs 7f
	rsb r7, r5, #32
	lsls r6, r6, r5
	lsr r8, lr, r7
	orrs r6, r6, r8
	lsls lr, lr, r5
	lsr r8, ip, r7
	orrs lr, lr, r8
	movs ip, #0
	b .Lpack
7:
	subs r5, r5, #32
	rsb r7, r5, #32
	lsl r6, lr, r5
	lsr r8, ip, r7
	orrs r6, r6, r8
	lsl lr, ip, r5
	movs ip, #0
	b .Lpack

.Lzero:
	movs r0, #0
	movs r1, #0
	pop {r4-r8, pc}

.Lsmall:
	/* b is a zero: x + 0 is x, and two zeros give -0 only if both are */
	orrs ip, r2, r3, lsl #1
	bne 8f
	orrs ip, r0, r1, lsl #1
	it eq
	andeq r1, r1, r3
	pop {r4-r8, pc}
8:
	/* b is subnormal: no hidden bit, the exponent of field 1 */
	cbz r4, 9f
	ubfx r7, r3, #0, #20
	subs r5, r4, #1
	b .Lalign
9:
	/* Both are subnormal: the magnitudes add or subtract exactly as integers, a carry reaching field 1 */
	eor ip, r1, r3
	and r6, r1, #0x80000000
	bic r1, r1, #0x80000000
	bic r3, r3, #0x80000000
	cmp ip, #0
	blt 10f
	adds r0, r0, r2
	adc r1, r1, r3
	orr r1, r1, r6
	pop {r4-r8, pc}
10:
	subs r0, r0, r2
	sbc r1, r1, r3
	orrs ip, r0, r1
	it ne
	orrne r1, r1, r6
	pop {r4-r8, pc}

.Linfnan:
	/* a is a NaN (the larger payload, if both are), quietened */
	orrs ip, r0, r1, lsl #12
	itt ne
	orrne r1, r1, #0x00080000
	popne {r4-r8, pc}

	/* a is an infinity: minus an infinity is invalid (7.2), giving the default NaN; anything else gives a */
	lsls ip, r3, #1
	cmp ip, #0xffe00000
	it eq
	cmpeq r2, #0
	bne 11f
	teq r1, r3
	bpl 11f
	movs r1, #0x7f000000
	orr r1, r1, #0x00f80000
11:
	pop {r4-r8, pc}
	.size __aeabi_dadd, . - __aeabi_dadd
