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

#if BOOT_STAGE != 1 && BOOT_STAGE != 2
#error Error reporting via DME is only supported for first two stage of boot
#endif

static uint32_t fw_errno;


/**
 * @brief Bootloader-specific "errno" wrapper initializer
 */
void init_last_error(void) {
    /*
     * 1st level fw will erase all errno fields. Subsequent levels
     * will only erase their level's field.
     */
#if BOOT_STAGE == 1
    fw_errno = BRE_OK;
#else
    {
        if (chip_unipro_attr_read(DME_DDBL2_INIT_STATUS, &fw_errno, 0,
                                  ATTR_LOCAL) != 0)
        {
            dbgprint("S2: init_last_error: can't read Boot Status\n");
            fw_errno = BRE_OK;
        }
    }
    /* strip off any DME status... */
    fw_errno &= INIT_STATUS_ERROR_CODE_MASK;

    /* ...and ensure that this level's field is cleared. */
    fw_errno &= ~BRE_S2_FW_MASK;
#endif
}


/**
 * @brief Wrapper to set the bootloader-specific "errno" value
 *
 * Note: The first error is sticky (subsequent settings are ignored)
 *
 * @param errno A BRE_xxx error code to save
 */
void set_last_error(uint32_t err) {
    uint32_t error_mask;
    uint32_t shift;
#if BOOT_STAGE == 1
    uint32_t boot_status;
    boot_status = chip_get_boot_status();

    boot_status &= INIT_STATUS_STATUS_MASK | INIT_STATUS_ERROR_MASK;
    if (boot_status == INIT_STATUS_FALLLBACK_UNIPRO_BOOT_STARTED) {
        error_mask = BRE_S1_FALLBACK_MASK;
        shift = BRE_S1_FALLBACK_SHIFT;
    } else {
        error_mask = BRE_S1_PAIMARY_MASK;
        shift = BRE_S1_PRIMARY_SHIFT;
    }
#elif BOOT_STAGE == 2
    error_mask = BRE_S2_FW_MASK;
    shift = BRE_S2_FW_SHIFT;
#endif

    if ((fw_errno & error_mask) == BRE_OK) {
        /*
         * Shift and mask the error based on BOOT_STAGE
         * and save the first error in each L1,2,3 reporting zone
         */
        fw_errno |= (err << shift) & error_mask;

#ifdef _DEBUGMSGS
        /* Save the first error in each S1,S2 reporting zone */
        if (err & (BRE_S1_PAIMARY_MASK | BRE_S1_FALLBACK_MASK)) {
            uint32_t    err_group = err & BRE_GROUP_MASK;
            /* Print out the error */
            dbgprintx32((err_group == BRE_EFUSE_BASE)? "S1 e-Fuse err: ":
                        (err_group == BRE_TFTF_BASE)? "S1 TFTF err: ":
                        (err_group == BRE_FFFF_BASE)? "S1 FFFF err: " :
                        (err_group == BRE_CRYPTO_BASE)? "S1 Crypto err: " :
                                "S1 error: ",
                                err, "\n");

        } else if (err & BRE_S2_FW_MASK) {
             dbgprintx32("S2 err: ", err, "\n");
        }
#endif
    }
}


/**
 * @brief Wrapper to get the last bootloader-specific "errno" value
 *
 * @returns The last BRE_xxx error code saved.
 */
uint32_t get_last_error(void) {
    return fw_errno;
}

/**
 * @brief Merge the bootrom "errno" and the boot status
 *
 *
 * @param boot_status The boot_status to push out to the DME variable
 * @return The merged boot_status variable
 */
uint32_t merge_errno_with_boot_status(uint32_t boot_status) {
    /* Since the boot has failed, add in the "boot failed" bit and the
     * global bootrom "errno", containing the details of the failure to
     * whatever boot status we've reached thus far, publish it via DME
     * and stop.
     */
    return (boot_status & ~INIT_STATUS_ERROR_CODE_MASK) |
           (get_last_error() & INIT_STATUS_ERROR_CODE_MASK);
}


/**
 * @brief Wrapper to set the boot status and stop
 *
 * This is a terminal execution node. All passengers must disembark
 *
 * @param errno A BRE_xxx error code to save
 */
void halt_and_catch_fire(uint32_t boot_status) {
    /* Since the boot has failed, add in the "boot failed" bit and the
     * global bootrom "errno", containing the details of the failure to
     * whatever boot status we've reached thus far, publish it via DME
     * and stop.
     */
    boot_status = merge_errno_with_boot_status(boot_status) |
                  INIT_STATUS_FAILED;
    dbgprintx32("Boot failed (", boot_status, ") halt\n");
    dbgflush();
    /* NOTE: NO FURTHER DEBUG MESSAGES BETWEEN HERE AND FUNCTION END! */

    if (!boot_status_offline)
        chip_advertise_boot_status(boot_status);

    /*
     * Indicate failure with GPIO 18 showing a '1' and execute a handshake
     * cycle on GPIO 16,17
     */
    chip_handshake_boot_status(boot_status);
    while(1);
}
