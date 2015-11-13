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
.file		"bootstrap.S"

.extern _bootstrap_lma, _bootstrap_vma, _bootstrap_size

/**
 * Under all chips prior to ES3, the bootrom is located in slow memory-mapped
 * SPI flash, and has to load itself into work RAM for performance.  Once this
 * bootstrap section has been loaded into RAM, the CPU jumps into it, and it
 * loads the rest of the bootrom into RAM.  The CPU then finally jumps into the
 * real bootrom.
 *
 * This code is located in a separate file due to its being in a separate linker
 * section.
 */

.section .bootstrap, "ax"
.align 4

.global copy_to_ram

.thumb_func
copy_to_ram:
    ldr r3, [r1], #4
    str r3, [r2], #4
    cbz r0, done_copy_to_ram
    sub r0, r0, #4
    b copy_to_ram
done_copy_to_ram:
    b bootrom_main

.align 4
_copy_to_ram:
