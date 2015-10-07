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
#include "bootrom.h"
#include "error.h"
#include "efuse.h"
#include "unipro.h"
#include "chipdef.h"
#include "debug.h"
#include "data_loading.h"
#include "tftf.h"
#include "ffff.h"
#include "crypto.h"
#include "bootrom.h"

extern data_load_ops spi_ops;
extern data_load_ops greybus_ops;


/**
 * @brief Stage 2 loader "C" entry point. Started from Stage 1
 * bootloader. Primary function is to load, validate, and start
 * executing a stage 3 image. Also will (when fully implemented)
 * perform startup negotiations with AP, cryptographic initialzations
 * and tests, module authentication, flash update, and other housekeeping.
 * Image load and validation are essntially identical to the crresponding
 * functions in stage 1, although different keys are used for signature
 * validation.
 *
 * @param none
 *
 * @returns Nothing. Will launch/restart image if successful, halt if not.
 */
void bootrom_main(void) {
    int rc;
    /* TA-20 R/W data in bufRAM */
    uint32_t    boot_status = INIT_STATUS_OPERATING;
    uint32_t    register_val;
    bool        boot_from_spi = true;
    bool        fallback_boot_unipro = false;
    uint32_t    is_secure_image;

    chip_init();

    dbginit();

    /* Ensure that we start each boot with an assumption of success */
    init_last_error();

    crypto_init();

    dbgprint("\nHello world from s2fw\n");

    chip_unipro_init();

    /* Advertise our boot status */
    chip_advertise_boot_status(boot_status);
    /* Advertise our initialization type */
    rc = chip_advertise_boot_type();
    if (rc) {
        halt_and_catch_fire(boot_status);
    }

    /*
     * Validate and make available e-fuse information (it handles error
     * reporting). Note that an error here is unrecoverable.
     */
    if (efuse_init() != 0) {
        halt_and_catch_fire(boot_status);
    }

    /* determine if we're booting from flash or unipro */
    register_val = tsb_get_bootselector();
#ifndef BOOT_OVER_UNIPRO
    register_val = 0;
#endif
   /* TA-02 Set SPIM_BOOT_N pin and read SPIBOOT_N register */
    boot_from_spi = ((register_val & TSB_EBOOTSELECTOR_SPIBOOT_N) == 0);
    if (boot_from_spi) {
        dbgprint("Boot from SPIROM\n");

        spi_ops.init();

        /**
         * Call locate_ffff_element_on_storage to locate next stage FW.
         * Do not care about the image length here so pass NULL.
         */
        if (locate_ffff_element_on_storage(&spi_ops,
                                           FFFF_ELEMENT_STAGE_3_FW,
                                           NULL) == 0) {
            boot_status = INIT_STATUS_SPI_BOOT_STARTED;
            chip_advertise_boot_status(boot_status);
            if (!load_tftf_image(&spi_ops, &is_secure_image)) {
                spi_ops.finish(true, is_secure_image);
                if (is_secure_image) {
                    boot_status = INIT_STATUS_TRUSTED_SPI_FLASH_BOOT_FINISHED;
                    dbgprintx32("SPI Trusted: (",
                                merge_errno_with_boot_status(boot_status),
                                ")\n");
                } else {
                    boot_status = INIT_STATUS_UNTRUSTED_SPI_FLASH_BOOT_FINISHED;
                    dbgprintx32("SPI Untrusted: (",
                                merge_errno_with_boot_status(boot_status),
                                ")\n");

                    /*
                     *  Disable IMS, CMS access before starting untrusted image.
                     *  NB. JTAG continues to be not enabled at this point
                     */
                    efuse_rig_for_untrusted();
                }

                /* Log that we're starting the boot-from-SPIROM */
                chip_advertise_boot_status(merge_errno_with_boot_status(boot_status));
                /* TA-16 jump to SPI code (BOOTRET_o = 0 && SPIBOOT_N = 0) */
                jump_to_image();
            }
        }
        spi_ops.finish(false, false);

        /* Fallback to UniPro boot */
        boot_from_spi = false;
        fallback_boot_unipro = true;

        chip_clear_image_loading_ram();
    } else {
        /* (Not boot-from-spi, */
        fallback_boot_unipro = false;
    }

    /* Boot-Over-UniPro...
     * We get here if directed to do so by the bootselector, or as a fallback
     * for a failed SPIROM boot.
     */
    if (!boot_from_spi) {
       /* Boot over Unipro */
        if (fallback_boot_unipro) {
            boot_status = merge_errno_with_boot_status(
                            INIT_STATUS_FALLLBACK_UNIPRO_BOOT_STARTED);
            dbgprintx32("Spi boot failed (", boot_status, "), ");
        } else {
            boot_status = INIT_STATUS_UNIPRO_BOOT_STARTED;
        }
        chip_advertise_boot_status(boot_status);
        dbgprintx32("Boot over UniPro (",
                    merge_errno_with_boot_status(boot_status),
                    ")\n");
        advertise_ready();
        dbgprint("Ready-poked; download-ready\n");
        if (greybus_ops.init() != 0) {
            halt_and_catch_fire(boot_status);
        }
        if (!load_tftf_image(&greybus_ops, &is_secure_image)) {
            if (greybus_ops.finish(true, is_secure_image) != 0) {
                halt_and_catch_fire(boot_status);
            }
            if (is_secure_image) {
                boot_status = fallback_boot_unipro ?
                    INIT_STATUS_FALLLBACK_TRUSTED_UNIPRO_BOOT_FINISHED :
                    INIT_STATUS_TRUSTED_UNIPRO_BOOT_FINISHED;
                dbgprintx32("UP Trusted: (",
                            merge_errno_with_boot_status(boot_status),
                            ")\n");
            } else {
                boot_status = fallback_boot_unipro ?
                    INIT_STATUS_FALLLBACK_UNTRUSTED_UNIPRO_BOOT_FINISHED :
                    INIT_STATUS_UNTRUSTED_UNIPRO_BOOT_FINISHED;
                dbgprintx32("UP Trusted: (",
                            merge_errno_with_boot_status(boot_status),
                            ")\n");

                /*
                 *  Disable IMS, CMS access before starting
                 * untrusted image
                 *  NB. JTAG continues to be not enabled at this point
                 */
                efuse_rig_for_untrusted();
            }

            /* Log that we're starting the boot-from-UniPro */
            chip_advertise_boot_status(boot_status);
            /* TA-17 jump to Workram code (BOOTRET_o = 0 && SPIM_BOOT_N = 1) */
            jump_to_image();
        }
        if (greybus_ops.finish(false, is_secure_image) != 0) {
            halt_and_catch_fire(boot_status);
        }
    }

    /* If we reach here, we didn't find an image to boot - stop while we're
     * ahead...
     */
    halt_and_catch_fire(boot_status);
}
