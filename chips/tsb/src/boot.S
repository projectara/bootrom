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

.syntax		unified
.file		"boot.S"

.text
.thumb
.thumb_func
.align	2

.globl bootrom_main

.globl _vector
_vectors:
	.word _stack_top
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
    /* clear workram */
    /* WARNING!!!: Assumption here is that workram will always be
       a multiple of 32 bytes. If this become not true for some interesting
       reason, we have to modify the code here. */
    ldr r0, =0
    mov r1, r0
    mov r2, r0
    mov r3, r0
    mov r4, r0
    mov r5, r0
    mov r6, r0
    mov r7, r0
    ldr r8, =_workram_start
    ldr r9, =_workram_end
clear_workram:
    stmia r8!, {r0, r1, r2, r3, r4, r5, r6, r7}
    cmp r8, r9
    bmi clear_workram

    /* initialize .data section */
    ldr r1, =_data_in_rom
    ldr r2, =_sdata
    ldr r3, =_edata
init_data_sec:
    cmp r2, r3
    bge end_init_data_sec
    ldr r0, [r1], #4
    str r0, [r2], #4
    b init_data_sec
end_init_data_sec:
	b bootrom_main

.type exception_common_handler, %function
exception_common_handler:
	b exception_common_handler

.globl chip_jump_to_image
.type chip_jump_to_image, %function
chip_jump_to_image:
    orr r0, #1    /* set Thumb mode for bx */
    bx r0

#ifndef BOOT_FROM_ROM
.globl	Reset_Handler
.type	Reset_Handler, %function
Reset_Handler:
    ldr sp, =_stack_top
    b __start
#endif
