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
 * @brief: This file contains the ISAA register access routines.
 *
 * (ISAA = Information Security And Assurance register)
 *
 * External documents:
 *      "ARA_ES3_APBridge_AppendixD_rev010.pdf
 *      "ARA_ES3_APBridge_RegisterMap_AppendixD_rev001.pdf
 *
 * Author: Morgan Girling <morgang@bsquare.com>
 */

#include <stdbool.h>
#include "tsb_isaa.h"


/* ISAA register offsets (relative to ISAA_BASE) */
#define TSB_ISAA_RNDO                       0x00000000
#define TSB_ISAA_RNDREADY                   0x00000004
#define TSB_ISAA_DEBUG0                     0x0000000c
#define TSB_ISAA_DEBUG1                     0x00000010
#define TSB_ISAA_DEBUG2                     0x00000014
#define TSB_ISAA_DEBUG3                     0x00000018
#define TSB_ISAA_DEBUG4                     0x0000001c
#define TSB_ISAA_DEBUG5                     0x00000020
#define TSB_ISAA_DEBUG6                     0x00000024
#define TSB_ISAA_DEBUG7                     0x00000028
#define TSB_ISAA_DEBUG8                     0x0000002c
#define TSB_ISAA_DEBUG9                     0x00000030
#define TSB_ISAA_DEBUG10                    0x00000040
#define TSB_ISAA_DEBUG11                    0x00000044
#define TSB_ISAA_DEBUG12                    0x00000050
#define TSB_ISAA_DEBUG13                    0x00000054
#define TSB_ISAA_DEBUG14                    0x00000060
#define TSB_ISAA_DEBUG15                    0x00000064
#define TSB_ISAA_DEBUG16                    0x00000068
#define TSB_ISAA_DEBUG17                    0x0000006c
#define TSB_ISAA_DEBUG18                    0x00000070
#define TSB_ISAA_DEBUG19                    0x00000074
#define TSB_ISAA_IMS0                       0x00000100
#define TSB_ISAA_IMS1                       0x00000104
#define TSB_ISAA_IMS2                       0x00000108
#define TSB_ISAA_IMS3                       0x0000010c
#define TSB_ISAA_IMS4                       0x00000110
#define TSB_ISAA_IMS5                       0x00000114
#define TSB_ISAA_IMS6                       0x00000118
#define TSB_ISAA_IMS7                       0x0000011c
#define TSB_ISAA_IMS8                       0x00000120
#define TSB_ISAA_CMS0                       0x00000200
#define TSB_ISAA_CMS1                       0x00000204
#define TSB_ISAA_CMS2                       0x00000208
#define TSB_ISAA_CMS3                       0x0000020c
#define TSB_ISAA_CMS4                       0x00000210
#define TSB_ISAA_CMS5                       0x00000214
#define TSB_ISAA_CMS6                       0x00000218
#define TSB_ISAA_SN0                        0x00000300
#define TSB_ISAA_SN1                        0x00000304
#define TSB_ISAA_SCR                        0x00000308
    #define TSB_ISAA_ROM_KEY_0_INVALID        (1 << 0)
    #define TSB_ISAA_ROM_KEY_1_INVALID        (1 << 1)
    #define TSB_ISAA_ROM_KEY_2_INVALID        (1 << 2)
    #define TSB_ISAA_ROM_KEY_3_INVALID        (1 << 3)
    #define TSB_ISAA_TRUSTED_ONLY             (1 << 4)
    #define TSB_ISAA_DISABLE_HARDWARE_CRYPTO  (1 << 7)
    #define TSB_ISAA_ROM_KEY_VALIDITY         0x0f
    #define TSB_ISAA_SCR_RESERVED             0x60
#define TSB_ISAA_RETEST                     0x0000030c
#define TSB_ISAA_DISABLE_IMS_ACCESS         0x00000400
#define TSB_ISAA_DISABLE_CMS_ACCESS         0x00000404
#define TSB_ISAA_JTAG_ENABLE                0x00000408
#define TSB_ISAA_JTAG_DISABLE               0x0000040c
#define TSB_ISAA_SOFTWARE_JTAG_CONTROL      0x00000410
#define TSB_ISAA_SELECT_TRANSFER_MODE       0x00000414
#define TSB_ISAA_TRANSFER_MODE01_DISABLE    0x00000418


/* DISABLE_IMS_ACCESS bits */
#define TSB_DISABLE_IMS_ACCESS  (1 << 0)

/* DISABLE_CMS_ACCESS bits */
#define TSB_DISABLE_CMS_ACCESS  (1 << 0)

/* DISABLE_CMS_ACCESS bits */
#define TSB_JTAG_DISABLE        (1 << 0)


/**
 * @brief Read a single 32-bit ISAA register
 *
 * @param offset The offset in bytes from ISAA_BASE.
 *
 * @returns Nothing
 */
static uint32_t isaa_read(uint32_t offset) {
    return getreg32(ISAA_BASE + offset);
}


/**
 * @brief Read a contiguous block of 32-bit ISAA registers
 *
 * @param offset The offset in bytes from ISAA_BASE.
 * @param buf Points to the buffer to which the registers will be copied.
 * @param size The size in bytes of buf,
 *
 * @returns Nothing
 */
static void isaa_read_n(uint32_t offset, uint8_t *buf, uint32_t size) {
    /* Copy whole registers directly into the buffer */
    while (size > sizeof(uint32_t)) {
        *(uint32_t *)buf = getreg32(ISAA_BASE + offset);
        buf += sizeof(uint32_t);
        offset += sizeof(uint32_t);
        size -= sizeof(uint32_t);
    }

    /* Copy any remaining bytes from the last register (LSB first) */
    if (size > 0) {
        uint32_t    temp;

        temp = getreg32(ISAA_BASE + offset);
        while (size > 0) {
            *buf++ = (uint8_t)temp;
            temp >>= 8;
            size--;
        }
    }
}


/**
 * @brief Write a single 32-bit ISAA register
 *
 * @param offset The offset in bytes from ISAA_BASE.
 * @param v The value to write to the register.
 *
 * @returns Nothing
 */
static void isaa_write(uint32_t offset, uint32_t v) {
    putreg32(v, ISAA_BASE + offset);
}


uint64_t tsb_get_serial_no(void) {
    uint64_t  temp;

    isaa_read_n(TSB_ISAA_SN0, (uint8_t *)&temp, sizeof(temp));
    return temp;
}


void tsb_get_ims(uint8_t * buf, uint32_t size) {
    if (size > TSB_ISAA_NUM_IMS_BYTES) {
        size = TSB_ISAA_NUM_IMS_BYTES;
    }
    isaa_read_n(TSB_ISAA_IMS0, buf, size);
}


void tsb_disable_ims_access(void) {
    isaa_write(TSB_ISAA_DISABLE_IMS_ACCESS, TSB_DISABLE_IMS_ACCESS);
}


/* TA-09 Lock function of eFuse (CMS) */
void tsb_disable_cms_access(void) {
    isaa_write(TSB_ISAA_DISABLE_CMS_ACCESS, TSB_DISABLE_CMS_ACCESS);
}

/* Those functions are not used in boot ROM, explicitly exclude them */
#if BOOT_STAGE != 1
void tsb_get_cms(uint8_t * buf, uint32_t size) {
    if (size > TSB_ISAA_NUM_CMS_BYTES) {
        size = TSB_ISAA_NUM_CMS_BYTES;
    }
    isaa_read_n(TSB_ISAA_CMS0, buf, size);
}


void tsb_enable_ims_access(void) {
    isaa_write(TSB_ISAA_DISABLE_IMS_ACCESS, 0);
}


void tsb_enable_cms_access(void) {
    isaa_write(TSB_ISAA_DISABLE_CMS_ACCESS, 0);
}
#endif


void tsb_jtag_disable(void) {
    isaa_write(TSB_ISAA_JTAG_DISABLE, TSB_JTAG_DISABLE);
}

int chip_is_key_revoked(int index) {
#if CONFIG_CHIP_REVISION >= CHIP_REVISION_ES3
    uint32_t scr = isaa_read(TSB_ISAA_SCR);

    if ((scr & (1 << index)) != 0) {
        return 1;
    }
#endif
    return 0;
}

bool chip_is_untrusted_image_allowed(void) {
#if CONFIG_CHIP_REVISION >= CHIP_REVISION_ES3
    uint32_t scr = isaa_read(TSB_ISAA_SCR);
    return !(scr & TSB_ISAA_TRUSTED_ONLY);
#endif
    return true;
}

uint32_t tsb_get_scr(void) {
    return isaa_read(TSB_ISAA_SCR);
}


/* TODO: Remove after bootloader completion if not used */
uint32_t tsb_get_retest(void) {
    return isaa_read(TSB_ISAA_RETEST);
}


/* TODO: Remove after bootloader completion if not used */
uint32_t tsb_get_jtag_enable(void) {
    return isaa_read(TSB_ISAA_JTAG_ENABLE);
}


/* TODO: Remove after bootloader completion if not used */
uint32_t tsb_get_software_jtag_control(void) {
    return isaa_read(TSB_ISAA_SOFTWARE_JTAG_CONTROL);
}


