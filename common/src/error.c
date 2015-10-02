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


/**
 * @brief Bootloader-specific "errno" wrapper initializer
 */
void init_last_error(void) {
    communication_area *p = (communication_area *)&_communication_area;

    /*
     * 1st level fw will erase all errno fields. Subsequent levels
     * will only erase their level's field.
     */
#if BOOT_STAGE == 1
    p->fw_errno = BRE_OK;
#else
    /* strip off any DME status... */
    p->fw_errno &= INIT_STATUS_ERROR_CODE_MASK;

    /* ...and ensure that this level's field is cleared. */
#if BOOT_STAGE == 2
    p->fw_errno &= ~BRE_L2_FW_MASK;
#elif BOOT_STAGE == 3
    p->fw_errno &= ~BRE_L3_FW_MASK;
#endif
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
    communication_area *p = (communication_area *)&_communication_area;
    uint32_t error_mask;
#if BOOT_STAGE == 1
    error_mask = BRE_L1_FW_MASK;
#elif BOOT_STAGE == 2
    error_mask = BRE_L2_FW_MASK;
#elif BOOT_STAGE == 3
    error_mask = BRE_L3_FW_MASK;
#endif

    if ((p->fw_errno & error_mask) == BRE_OK) {

        /* Automatically shift and mask the error based on BOOT_STAGE */
#if BOOT_STAGE == 1
        err = (err << BRE_L1_FW_SHIFT) & error_mask;
#elif BOOT_STAGE == 2
        err = (err << BRE_L2_FW_SHIFT) & error_mask;
#elif BOOT_STAGE == 3
        err = (err << BRE_L3_FW_SHIFT) & error_mask;
#endif

        /* Save the first error in each L1,2,3 reporting zone */
        if (err & BRE_L1_FW_MASK) {
            /* level 1 error */
            if ((p->fw_errno & BRE_L1_FW_MASK) == 0) {
                p->fw_errno |= err & BRE_L1_FW_MASK;
            }
            uint32_t    err_group = err & BRE_GROUP_MASK;
            /* Print out the error */
            dbgprintx32((err_group == BRE_EFUSE_BASE)? "L1 e-Fuse err: ":
                        (err_group == BRE_TFTF_BASE)? "L1 TFTF err: ":
                        (err_group == BRE_FFFF_BASE)? "L1 FFFF err: " :
                        (err_group == BRE_CRYPTO_BASE)? "L1 Crypto err: " :
                                "L1 error: ",
                                err, "\n");

        } else if (err & BRE_L2_FW_MASK) {
            /* level 2 error */
            if ((p->fw_errno & BRE_L2_FW_MASK) == 0) {
                p->fw_errno |= err & BRE_L2_FW_MASK;
                dbgprintx32("L2 err: ", err, "\n");
            }
       } else  {
            /* level 3 error */
           if ((p->fw_errno & BRE_L3_FW_MASK) == 0) {
               p->fw_errno |= err & BRE_L3_FW_MASK;
               dbgprintx32("L3 err: ", err, "\n");
           }
        }
    }
}


/**
 * @brief Wrapper to get the last bootloader-specific "errno" value
 *
 * @returns The last BRE_xxx error code saved.
 */
uint32_t get_last_error(void) {
    communication_area *p = (communication_area *)&_communication_area;

    return p->fw_errno;
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

    if (!boot_status_offline) {
        chip_advertise_boot_status(boot_status);
    }

#if defined(_SIMULATION) && ((BOOT_STAGE == 1) || (BOOT_STAGE == 3))
    /*
     * Indicate failure with GPIO 18 showing a '1' and execute a handshake
     * cycle on GPIO 16,17
     */
    chip_handshake_boot_status(boot_status);
#endif
    while(1);
}
