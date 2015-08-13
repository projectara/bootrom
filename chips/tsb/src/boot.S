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

#include "chipdef.h"

.text
.thumb
.thumb_func
.align	2

.globl bootrom_main

.globl	Reset_Handler
.type	Reset_Handler, %function
Reset_Handler:
    ldr sp, =_stack_top
    b bootrom_main

.globl chip_jump_to_image
.type chip_jump_to_image, %function
chip_jump_to_image:
    orr r0, #1    /* set Thumb mode for bx */
    bx r0

#if BOOT_STAGE == 1
.type exception_common_handler, %function
exception_common_handler:
	b exception_common_handler

.section .vectors
.globl _vectors
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

/* experiment on ES1 shows this is enough for 0.1us */
#define DELAY_100NS_LOOP_COUNT 3

#ifdef BOOT_FROM_SLOW_ROM
.section .bootstrap.loader, "ax"
#else
.text
#endif

#ifdef BOOT_FROM_SLOW_ROM
.globl _bootstrap_vectors
_bootstrap_vectors:
	.word _stack_top
	.word __start
#endif

.globl __start
.type __start, %function
__start:
# TA-01 The Boot ROM begins executing at address 0x00000000...
#if CONFIG_CHIP_REVISION >= CHIP_REVISION_ES3
    ldr r1, =BOOTRET_O
    ldr r0, [r1]
    tst r0, #1
    beq cold_boot

    /*
     * "Resume" path
     * TA-06 Write/Read registers to resume standby
     */
    ldr r2, =RETSRAMRET
    ldr r0, =0
    str r0, [r2]

    ldr r3, =DELAY_100NS_LOOP_COUNT
delay_100ns_loop:
    subs r3, r3, #1
    bne delay_100ns_loop

    ldr r2, =RETSRAMCLKCONT
    ldr r0, =1
    str r0, [r2]

    /* delay 5 CPU cycles or more */
    nop
    nop
    nop
    nop
    nop

    ldr r2, =RETSRAMCENCONT
    ldr r0, =0
    str r0, [r2]

    /*
     * Check for the resume address and its complement before jumping to it
     */
    ldr r2, =_resume_address
    ldm r2, {r0, r1}
    mvn r1, r1
    cmp r0, r1   /* check the complement of the resume address */
    bne cold_boot

    /*
     * Before resuming the higher-level code, ensure that IMS and CMS
     * and JTAG are disabled.
     */
    ldr r0, = DISABLE_JTAG_IMS_CMS_ACCESS
    ldr r3, =_DISABLE_IMS_ACCESS
    str r0, [r3]
    ldr r3, = _DISABLE_CMS_ACCESS
    str r0, [r3]
    ldr r3, = _JTAG_DISABLE
    str r0, [r3]


    ldr r3, =0
    str r3, [r2], #4 /* clear the resume address */
    str r3, [r2]
    orr r0, #1   /* make sure thumb bit is set */
    bx r0   /* jump to resume address */

cold_boot:
#endif
#if CONFIG_CHIP_REVISION < CHIP_REVISION_ES3
    // ES2's ARM core doesn't seem to load the correct stack pointer
    ldr sp, =_stack_top
#endif
    /* Clear workram
     *
     * WARNING!!!: Assumption here is that workram will always be
     * a multiple of 32 bytes. If this becomes untrue for some interesting
     * reason, we have to modify the code here.
     *
     * NOTE: We do not clear workram if we're running on a chip simulator,
     * because the simulator has already zapped memory and this loop slows
     * down the simulation.
     */
    ldr r0, =0
    mov r1, r0
    mov r2, r0
    mov r3, r0
    mov r4, r0
    mov r5, r0
    mov r6, r0
    mov r7, r0
    /* TA-10 Write/Read data in workRAM */
#ifdef _SIMULATION
    /* Just clear the BootRom .data, .bss, stack and communication region*/
    ldr r8, =_bootrom_data_area
#else
    /* Clear all of work RAM */
    ldr r8, =_workram_start
#endif
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
#ifdef BOOT_FROM_SLOW_ROM
	b bootstrap
#else
	b bootrom_main
#endif

#ifdef BOOT_FROM_SLOW_ROM
.globl bootstrap
.extern copy_to_ram

bootstrap:
    ldr r0, =_bootstrap_lma
    ldr r1, =_bootstrap_size
    ldr r2, =_bootstrap_vma

bootstrap_copy:
    cbz r1, done_bootstrap_copy
    ldr r3, [r0], #4
    str r3, [r2], #4
    sub r1, r1, #4
    b bootstrap_copy

done_bootstrap_copy:
    ldr r0, =_text_size
    add r0, r0, #3
    bic r0, r0, #3
    ldr r1, =_stext_lma
    ldr r2, =_stext
    b copy_to_ram
#endif
#endif /* BOOT_STAGE == 1 */
