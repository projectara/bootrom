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

#include <stdint.h>
#include "chipapi.h"
#include "tsb_scm.h"
#include "tsb_isaa.h"
#include "common.h"
#include "efuse.h"
#include "unipro.h"
#include "debug.h"
#include "data_loading.h"
#include "tftf.h"
#include "ffff.h"



extern data_load_ops spi_ops;


/**
 * @brief Bootloader "C" entry point
 *
 * @param none
 *
 * @returns Nothing. Will launch/restart image if successful, halt if not.
 */
void bootrom_main(void) {
    uint32_t    dme_write_result;
    uint32_t    boot_status = INIT_STATUS_OPERATING;
    uint32_t    register_val;
    int         status = 0;
    bool        boot_from_spi = true;
#ifdef BOOT_OVER_UNIPRO
    bool        fallback_boot_unipro = false;
#endif

    chip_init();

    dbginit();
    dbgprint("Hello world!\r\n");

    chip_unipro_init();

    /* Advertise our boot status */
    chip_unipro_attr_write(DME_DDBL2_INIT_STATUS, boot_status, 0,
                           ATTR_LOCAL, &dme_write_result);

    /* Advertise our initialization type */
    chip_unipro_attr_write(DME_DDBL2_INIT_TYPE, INIT_TYPE_TOSHIBA, 0,
                               ATTR_LOCAL, &dme_write_result);

    /* Validate and make available e-fuse information */
#if (CONFIG_CHIP_REVISION == CHIP_REVISION_ES3)
    status = efuse_init();
#endif
    if (0 != status) {
        dbgprint("bootrom_main: eFuse failure\r\n");
        boot_status |= status & INIT_STATUS_ERROR_CODE_MASK;
        goto halt_and_catch_fire;
    }

    register_val = tsb_get_bootselector();
#ifndef BOOT_OVER_UNIPRO
    dbgprint("Boot-Over-Unipro disabled, force force boot-from-SPI\r\n");
    register_val = 0;
#endif
   if ((register_val & TSB_EBOOTSELECTOR_SPIBOOT_N) == 0) {
        dbgprint("BOOTSELECTOR 0\r\n");
        boot_from_spi = true;
    } else {
        dbgprint("BOOTSELECTOR 1\r\n");
        boot_from_spi = false;
    }
    if (boot_from_spi) {
        dbgprint("bootrom_main: Boot from SPIROM\r\n");

        spi_ops.init();
        uint32_t is_secure_image;
        if (locate_second_stage_firmware_on_storage(&spi_ops) == 0) {
            boot_status = INIT_STATUS_SPI_BOOT_STARTED;
            chip_unipro_attr_write(DME_DDBL2_INIT_STATUS, boot_status, 0,
                                   ATTR_LOCAL, &dme_write_result);
            if (!load_tftf_image(&spi_ops, &is_secure_image)) {
                spi_ops.finish();
                if (is_secure_image) {
                    dbgprint("Trusted image\r\n");
                    boot_status = INIT_STATUS_TRUSTED_SPI_FLASH_BOOT_FINISHED;
                } else {
                    dbgprint("Untrusted image\r\n");
                    boot_status = INIT_STATUS_UNTRUSTED_SPI_FLASH_BOOT_FINISHED;

                    /*
                     *  Disable JTAG, IMS, CMS access before starting
                     * untrusted image
                     */
                    efuse_rig_for_untrusted();
                }
                /* Log that we're starting the boot-from-SPIROM */
                chip_unipro_attr_write(DME_DDBL2_INIT_STATUS, boot_status, 0,
                                       ATTR_LOCAL, &dme_write_result);
                jump_to_image();
            }
        }
        /*****/dbgprint("couldn't locate image\r\n");
        spi_ops.finish();

#ifdef BOOT_OVER_UNIPRO
        /* Fallback to UniPro boot */
        boot_from_spi = false;
        fallback_boot_unipro = false;

    } else {
        fallback_boot_unipro = true;
#endif
    }

#ifdef BOOT_OVER_UNIPRO
    /* Boot-Over-UniPro if directed to do so or as a fallback for a failed
     * SPIROM boot.
     */
    if (!boot_from_spi) {
        dbgprint("bootrom_main: Boot over UniPro\r\n");
        /* Boot over Uniprom */
        boot_status = fallback_boot_unipro?
                INIT_STATUS_FALLLBACK_UNIPRO_BOOT_STARTED :
                INIT_STATUS_UNIPRO_BOOT_STARTED;
        /***** BOU TBD ****/
    }
#endif


    /* Failure */
halt_and_catch_fire:
    boot_status |= INIT_STATUS_FAILED;
    chip_unipro_attr_write(DME_DDBL2_INIT_STATUS, boot_status, 0, ATTR_LOCAL,
                           &dme_write_result);
    dbgprint("failed to load image, entering infinite loop\r\n");
    while(1);
}
