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
#include "chipapi.h"
#include "tsb_scm.h"
#include "debug.h"
#include "data_loading.h"
#include "crypto.h"

static uint32_t current_addr;

#define SPIM_CTRLR0 (SPI_BASE)
#define SPIM_CTRLR1 (SPI_BASE + 0x04)
#define SPIM_SSIENR (SPI_BASE + 0x08)
#define SPIM_SER    (SPI_BASE + 0x10)
#define SPIM_BAUDR  (SPI_BASE + 0x14)
#define SPIM_SR     (SPI_BASE + 0x28)
#define SPIM_DR0    (SPI_BASE + 0x60)

/* CTRLR0 value: 32bit frame, EEPROM read mode */
#define SPIM_CTRLR0_VALUE 0x001F0300

#define SPIM_SR_RFNE 0x08
#define SPIM_SR_BUSY 0x01

/* SCKDV: 2 - 24MHz SPI CLK */
#define SPIM_SCKDV 2

#define SPIM_SSI_ENABLE  1
#define SPIM_SSI_DISABLE 0
/* SPI_CS_0_N */
#define SPIM_SLAVE_SELECT (1 << 0)

#define SPI_FLASH_READ_CMD 0x03

static int data_load_spi_init(void) {
    current_addr = 0;

    /* enable SPI master clock.
       Pinshare should be default to SPI (CS0) after reset */
    tsb_clk_enable(TSB_CLK_SPIP);
    tsb_clk_enable(TSB_CLK_SPIS);

    putreg32(SPIM_SSI_DISABLE,  SPIM_SSIENR);
    putreg32(SPIM_CTRLR0_VALUE, SPIM_CTRLR0);
    putreg32(SPIM_SCKDV,  SPIM_BAUDR);
    putreg32(SPIM_SLAVE_SELECT,  SPIM_SER);

    return 0;
}

/* TA-15 CM3 perform read data transfer from SPI memory to data transfer... */
static int data_load_spi_load(void *dest, uint32_t length, bool hash) {
    uint32_t c;
    uint32_t sr, dr;
    unsigned char *pdest = (unsigned char *)dest;
    unsigned char *pdr = (unsigned char *)&dr;
    uint32_t count = length >> 2;

    if (length == 0) {
        return 0;
    }

    /* only 24bits of address in the SPI read command, but address wrap around
       is actually leagal in SPI flash read. Since we don't really know the
       size of the SPI flash, we just let the wrap around happen and the data
       integrity check should catch the error */
    current_addr &= 0x00FFFFFF;

    if (count > 0xFFFF) {
        /* Maximum frame count is 64k (32bit frame). This would never be
           exceeded on the Toshiba bridge chip, since we only have 192kB
           continous RAM. The chip_validate_data_load_location would take care
           of that when validating image length. This check here is just in
           case the chip design is changed some day. */
        return -1;
    }

    putreg32(count - 1, SPIM_CTRLR1);
    putreg32(SPIM_SSI_ENABLE,  SPIM_SSIENR);
    putreg32((SPI_FLASH_READ_CMD << 24) | current_addr, SPIM_DR0);
    c = 0;
    while(1) {
        sr = getreg32(SPIM_SR);
        /* The spec says that "BUSY" doesn't happen right away with not much
           explaination. However, it should be safe to assume it would happen
           no later than the first frame is received */
        if (c && !(sr & (SPIM_SR_BUSY | SPIM_SR_RFNE))) {
            break;
        }
        if (sr & SPIM_SR_RFNE) {
            dr = getreg32(SPIM_DR0);
            *pdest++ = pdr[3];
            *pdest++ = pdr[2];
            *pdest++ = pdr[1];
            *pdest++ = pdr[0];
            c++;
        }
    }
    putreg32(SPIM_SSI_DISABLE,  SPIM_SSIENR);

    if (c != count) {
        /* During experiment, RX FIFO overflow was observed in certain
           conditions, so data loss happened. However, the boot ROM is running
           under fixed core and SPI clocks and single threaded. So once the
           code is finalized, the behavior of each party is predictable and
           this error should never happen.
           The check is just for cautious. */
        return -1;
    }

    count = length & 3;
    current_addr += (length & ~3);
    current_addr &= 0x00FFFFFF;

    if (0 != count) {
        /* read trailing bytes */
        putreg32(0, SPIM_CTRLR1);
        putreg32(SPIM_SSI_ENABLE,  SPIM_SSIENR);
        putreg32((SPI_FLASH_READ_CMD << 24) | current_addr, SPIM_DR0);
        while(1) {
            sr = getreg32(SPIM_SR);
            if (sr & SPIM_SR_RFNE) {
                dr = getreg32(SPIM_DR0);
                for (c = 0; c < count; c++) {
                    *pdest++ = pdr[3 - c];
                }
                break;
            }
        }
        putreg32(SPIM_SSI_DISABLE,  SPIM_SSIENR);
        current_addr += count;
    }

    if (hash) {
        hash_update((unsigned char *)dest, length);
    }
    return 0;
}

static int data_load_spi_read(void *dest, uint32_t addr, uint32_t length) {
    current_addr = addr;
    if (0 == length) {
        return 0;
    }

    return data_load_spi_load(dest, length, false);
}

static int data_load_spi_finish(bool valid, bool is_secure_image) {
    tsb_clk_disable(TSB_CLK_SPIP);
    tsb_clk_disable(TSB_CLK_SPIS);
    return 0;
}

data_load_ops spi_ops = {
    .init = data_load_spi_init,
    .read = data_load_spi_read,
    .load = data_load_spi_load,
    .finish = data_load_spi_finish
};
