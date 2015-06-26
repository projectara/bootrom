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

/*
 * Author: Benoit Cousson <bcousson@baylibre.com>
 *         Fabien Parent <fparent@baylibre.com>
 */

#include "tsb_scm.h"


#define TSB_SCM_SOFTRESET0              0x00000000
#define TSB_SCM_SOFTRESETRELEASE0       0x00000100
#define TSB_SCM_CLOCKGATING0            0x00000200
#define TSB_SCM_CLOCKENABLE0            0x00000300
#define TSB_SCM_BOOTSELECTOR            0x00000400
#define TSB_SCM_ECCERROR                0x000004c4
#define TSB_SCM_VID                     0x00000700
#define TSB_SCM_PID                     0x00000704
#define TSB_SCM_PINSHARE                0x00000800


static uint32_t scm_read(uint32_t offset) {
    return getreg32(SYSCTL_BASE + offset);
}

static void scm_write(uint32_t offset, uint32_t v) {
    putreg32(v, SYSCTL_BASE + offset);
}

uint32_t tsb_get_debug_reg(uint32_t offset)
{
    return scm_read(offset);
}

void tsb_clk_init(void) {
    scm_write(TSB_SCM_CLOCKGATING0, INIT_PERIPHERAL_CLOCK_BITS);
}

void tsb_clk_enable(uint32_t clk) {
    scm_write(TSB_SCM_CLOCKENABLE0 + CLK_OFFSET(clk), CLK_MASK(clk));
}

void tsb_clk_disable(uint32_t clk)
{
    scm_write(TSB_SCM_CLOCKGATING0 + CLK_OFFSET(clk), CLK_MASK(clk));
}

void tsb_reset(uint32_t rst) {
    scm_write(TSB_SCM_SOFTRESET0 + CLK_OFFSET(rst), CLK_MASK(rst));
    scm_write(TSB_SCM_SOFTRESETRELEASE0 + CLK_OFFSET(rst), CLK_MASK(rst));
}

void tsb_set_pinshare(uint32_t bits) {
    uint32_t r = scm_read(TSB_SCM_PINSHARE);
    scm_write(TSB_SCM_PINSHARE, r | bits);
}

uint32_t tsb_get_bootselector(void) {
    return scm_read(TSB_SCM_BOOTSELECTOR);
}

uint32_t tsb_get_eccerror(void) {
    return scm_read(TSB_SCM_ECCERROR);
}

uint32_t tsb_get_vid(void) {
    return scm_read(TSB_SCM_VID);
}

uint32_t tsb_get_pid(void) {
    return scm_read(TSB_SCM_PID);
}
