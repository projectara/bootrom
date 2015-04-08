/**
 * Copyright (c) 2015 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define BOOT_STACK		(0x10030000 - 4)

.syntax		unified
.file		"boot.S"

.text
.thumb
.thumb_func
.align	2

.globl bootrom_main

.globl _vector
_vectors:
	.word BOOT_STACK
	.word __start
	.word exception_common_handler
	.word exception_common_handler
	.word exception_common_handler
	.word exception_common_handler
	.word exception_common_handler
	.word exception_common_handler
	.word exception_common_handler
	.word exception_common_handler
	.word exception_common_handler
	.word exception_common_handler
	.word exception_common_handler
	.word exception_common_handler
	.word exception_common_handler
	.word exception_common_handler

.globl __start
.type __start, %function
__start:
	b bootrom_main

.type exception_common_handler, %function
exception_common_handler:
	b exception_common_handler

#ifndef BOOT_FROM_ROM
.globl	Reset_Handler
.type	Reset_Handler, %function
Reset_Handler:
    ldr sp, =BOOT_STACK
    b __start
#endif
