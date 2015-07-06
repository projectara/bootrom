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

#include <stddef.h>
#include "chip.h"
#include "chipapi.h"
#include "debug.h"
#include "tsb_scm.h"
#include "data_loading.h"
#include "crypto.h"

static uint8_t *current_addr = NULL;
static uint8_t initialized = 0;

static int data_load_mmapped_init(void) {
    if (initialized != 1) {
        current_addr = (uint8_t*) MMAP_LOAD_BASE;
        initialized = 1;

        /* enable SPI master clock.
           Pinshare should be default to SPI (CS0) after reset */
        tsb_clk_enable(TSB_CLK_SPIP);
        tsb_clk_enable(TSB_CLK_SPIS);
        return 0;
    }
    return -1;
}

static int data_load_mmapped_read(void *dest, uint32_t addr, uint32_t length) {
    if(initialized != 1 || addr + length >= MMAP_LOAD_SIZE)
        return -1;

    current_addr = (uint8_t*) (MMAP_LOAD_BASE + addr);
    uint8_t *dst;
    for (dst = (uint8_t*) dest;
            current_addr < (uint8_t*) (MMAP_LOAD_BASE + addr + length);)
        *dst++ = *current_addr++;

    return 0;
}

static int data_load_mmapped_load(void *dest, uint32_t length, bool hash) {
    if(initialized != 1 ||
       current_addr + length >= (uint8_t*)(MMAP_LOAD_BASE + MMAP_LOAD_SIZE))
        return -1;

    uint8_t *load_end = current_addr + length;
    uint8_t *dst;
    for (dst = (uint8_t*) dest; current_addr < load_end;)
        *dst++ = *current_addr++;

    if (hash) {
        hash_update((unsigned char *)dest, length);
    }
    return 0;
}

void data_load_mmapped_finish(void) {
    /* We do not disable the SPI master clock on ES1. */
    initialized = 0;
    current_addr = NULL;
}

data_load_ops spi_ops = {
    .init = data_load_mmapped_init,
    .read = data_load_mmapped_read,
    .load = data_load_mmapped_load,
    .finish = data_load_mmapped_finish
};
