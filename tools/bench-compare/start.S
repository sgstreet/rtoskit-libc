/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * start.S
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * Process entry for the comparison programs (tools/scripts/bench-runner
 * compare, P3.06): qemu-arm's Linux loader leaves argc at the stack pointer
 * and argv above it, maps the segments at their addresses and zeroes .bss,
 * so main is called directly and exit runs the library's atexit work.
 */
	.syntax unified
	.thumb
	.text
	.global _start
	.type _start, %function
_start:
	movs r0, #0
	mov lr, r0
	bl shim_init
	ldr r0, [sp]
	add r1, sp, #4
	bl main
	bl exit
	udf #0
	.size _start, . - _start

	.section .note.GNU-stack, "", %progbits
