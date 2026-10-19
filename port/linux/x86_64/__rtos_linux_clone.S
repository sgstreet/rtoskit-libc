/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_linux_clone.S
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * long __rtos_linux_clone(flags %rdi, stack %rsi, parent_tid %rdx, child_tid %rcx,
 *                         tls %r8, fn %r9, arg 8(%rsp))
 *
 * The kernel takes flags, stack, parent_tid, child_tid (%r10), tls. fn and
 * arg travel on the child's stack, which starts 16-byte aligned for the call.
 */
	.text
	.global __rtos_linux_clone
	.type __rtos_linux_clone, @function
__rtos_linux_clone:
	mov 8(%rsp), %rax
	and $-16, %rsi
	sub $16, %rsi
	mov %r9, 0(%rsi)
	mov %rax, 8(%rsi)
	mov %rcx, %r10
	mov $56, %eax
	syscall
	test %rax, %rax
	jz 1f
	ret
1:
	xor %ebp, %ebp
	pop %rax
	pop %rdi
	call *%rax
	mov %eax, %edi
	mov $60, %eax
	syscall
	hlt
	.size __rtos_linux_clone, . - __rtos_linux_clone

	.section .note.GNU-stack, "", @progbits
