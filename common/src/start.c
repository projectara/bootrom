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
#include <stdbool.h>
#include "chipapi.h"
#include "tsb_scm.h"
#include "tsb_isaa.h"
#include "common.h"
#include "error.h"
#include "efuse.h"
#include "unipro.h"
#include "chipdef.h"
#include "debug.h"
#include "data_loading.h"
#include "tftf.h"
#include "ffff.h"
#include "crypto.h"

extern data_load_ops spi_ops;
extern data_load_ops greybus_ops;

uint32_t br_errno;

/**
 * @brief Bootloader "C" entry point
 *
 * @param none
 *
 * @returns Nothing. Will launch/restart image if successful, halt if not.
 */
void bootrom_main(void) {
    uint32_t    dme_write_result;
    /* TA-20 R/W data in bufRAM */
    uint32_t    boot_status = INIT_STATUS_OPERATING;
    uint32_t    register_val;
    bool        boot_from_spi = true;
#ifdef BOOT_OVER_UNIPRO
    bool        fallback_boot_unipro = false;
#endif
    uint32_t is_secure_image;

    /* Ensure that we start each boot with an assumption of success */
    init_last_error();

    chip_init();

    dbginit();

    crypto_init();

#if BOOT_STAGE == 1
    dbgprint("Hello world from s1fw\n");
#elif BOOT_STAGE == 2
    dbgprint("Hello world from s2fw\n");
#elif BOOT_STAGE == 3
    dbgprint("Hello world from s3fw\n");
#ifdef _SIMULATION
    /* Handshake with the controller, indicating success */
    chip_signal_boot_status(0);
#endif
    /* Our work is done */
    while(1);
#endif

    chip_unipro_init();

    /* Advertise our boot status */
    chip_advertise_boot_status(boot_status, &dme_write_result);

    /* Advertise our initialization type */
    chip_advertise_boot_type(&dme_write_result);

    /*
     * Validate and make available e-fuse information (it handles error
     * reporting). Note that an error here is unrecoverable.
     */
    if (0 != efuse_init()) {
        goto halt_and_catch_fire;
    }

    register_val = tsb_get_bootselector();
#ifndef BOOT_OVER_UNIPRO
    register_val = 0;
#endif
   /* TA-02 Set SPIM_BOOT_N pin and read SPIBOOT_N register */
   if ((register_val & TSB_EBOOTSELECTOR_SPIBOOT_N) == 0) {
        boot_from_spi = true;
    } else {
        boot_from_spi = false;
    }
    if (boot_from_spi) {
        dbgprint("Boot from SPIROM\n");

        spi_ops.init();

        /**
         * Call locate_ffff_element_on_stage to locate next stage FW.
         * Do not care about the image length here so pass NULL.
         * The element type of next stage FW defined in FFFF happens to be
         * the same as BOOT_STAGE
         */
        if (locate_ffff_element_on_storage(&spi_ops, BOOT_STAGE, NULL) == 0) {
            boot_status = INIT_STATUS_SPI_BOOT_STARTED;
            chip_advertise_boot_status(boot_status, &dme_write_result);
            if (!load_tftf_image(&spi_ops, &is_secure_image)) {
                spi_ops.finish(true, is_secure_image);
                if (is_secure_image) {
                    dbgprint("Trusted image\n");
                    boot_status = INIT_STATUS_TRUSTED_SPI_FLASH_BOOT_FINISHED;
                } else {
                    dbgprint("Untrusted image\n");
                    boot_status = INIT_STATUS_UNTRUSTED_SPI_FLASH_BOOT_FINISHED;

                    /*
                     *  Disable JTAG, IMS, CMS access before starting
                     * untrusted image
                     */
                    efuse_rig_for_untrusted();
                }
                /* Log that we're starting the boot-from-SPIROM */
                chip_advertise_boot_status(boot_status, &dme_write_result);
                /* TA-16 jump to SPI code (BOOTRET_o = 0 && SPIBOOT_N = 0) */
                jump_to_image();
            }
        }
        /*****/dbgprint("No image\n");
        spi_ops.finish(false, false);

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
        int         status = 0;

        /* Boot over Uniprom */
        dbgprint("Boot over UniPro\n");
        boot_status = fallback_boot_unipro?
                INIT_STATUS_FALLLBACK_UNIPRO_BOOT_STARTED :
                INIT_STATUS_UNIPRO_BOOT_STARTED;
        chip_advertise_boot_status(boot_status, &dme_write_result);
        advertise_ready();
        dbgprint("Ready-poked; download-ready\n");
        status = greybus_ops.init();
        if (status)
            goto halt_and_catch_fire;
        if (!load_tftf_image(&greybus_ops, &is_secure_image)) {
            status = greybus_ops.finish(true, is_secure_image);
            if (status)
                goto halt_and_catch_fire;
            /* TA-17 jump to Workram code (BOOTRET_o = 0 && SPIM_BOOT_N = 1) */
            jump_to_image();
        }
        status = greybus_ops.finish(false, is_secure_image);
    }
#endif


    /* Failure */
halt_and_catch_fire:
    /* Since the boot has failed, add in the "boot failed" bit and the
     * global bootrom "errno", containing the details of the failure to
     * whatever boot status we've reached thus far, publish it via DME
     * and stop.
     */
    boot_status = (boot_status & ~INIT_STATUS_ERROR_CODE_MASK) |
                   (get_last_error() & INIT_STATUS_ERROR_CODE_MASK) |
                   INIT_STATUS_FAILED;
    dbgprintx32("Boot failed (", boot_status, ") halt\n");
    chip_advertise_boot_status(boot_status, &dme_write_result);

#if defined(_SIMULATION) && ((BOOT_STAGE == 1) || (BOOT_STAGE == 3))
    /*
     * Indicate failure with GPIO 18 showing a '1' and execute a handshake
     * cycle on GPIO 16,17
     */
    chip_signal_boot_status(boot_status);
#endif
    /* TODO: Change from while(1); to WFI? */
    while(1);
}


/**
 * @brief Wrapper to set the bootloader-specific "errno" value
 *
 * Note: The first error is sticky (subsequent settings are ignored)
 *
 * @param errno A BRE_xxx error code to save
 */
void set_last_error(uint32_t err) {
    if (br_errno == BRE_OK) {
        uint32_t    err_group = err & BRE_GROUP_MASK;

        /* Save the error */
        br_errno = err;
        /* Print out the error */
        dbgprintx32((err_group == BRE_EFUSE_BASE)? "e-Fuse err: ":
                    (err_group == BRE_TFTF_BASE)? "TFTF err: ":
                    (err_group == BRE_FFFF_BASE)? "FFFF err: " :
                    (err_group == BRE_CRYPTO_BASE)? "Crypto err: " : "error: ",
                    err, "\n");
    }
}
