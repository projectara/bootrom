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
#include <errno.h>
#include "chip.h"
#include "chipapi.h"
#include "unipro.h"
#include "tsb_unipro.h"
#include "debug.h"
#include "error.h"
#include "utils.h"

static bool boot_status_offline = false;

/**
 * @brief advertise the boot status
 * @param boot_status
 * @param result_code destination for advertisement result
 * @return 0 on success, <0 on error
 */
void chip_advertise_boot_status(uint32_t boot_status) {
    int rc;

    if (!boot_status_offline)
        return;

    rc = chip_unipro_attr_write(T_TSTSRCINCREMENT, ES2_INIT_STATUS(boot_status),
                                0, ATTR_LOCAL);
    /*
     * Being unable to write the DME value is regarded as a catastrophic failure.
     */
    if (rc) {
        /*
         * Set this flag so we can exit the recursive call from
         * halt_and_catch_fire
         */
        boot_status_offline = true;
        halt_and_catch_fire(boot_status);
    }
}

/**
 * @brief get the boot status from DME
 *        The reason for not using a global variable to track the boot status
 *        is in this way it can be cross boot stages.
 * @return boot_status (will call halt_and_catch_fire if unable to read boot
 *         status
 */
uint32_t chip_get_boot_status(void) {
    uint32_t boot_status;
    int rc;

    rc = chip_unipro_attr_read(T_TSTSRCINCREMENT, &boot_status,
                               0, ATTR_LOCAL);
    /*
     * Being unable to read the DME value is regarded as a catastrophic failure.
     */
    if (rc) {
        /*
         * Set this flag so that halt_and_catch_fire doesn't try to
         * recursively call us to advertise the boot status.
         */
        boot_status_offline = true;
        halt_and_catch_fire(boot_status);
    }

    return (boot_status << 24);
}

/**
 * @brief advertise the boot type
 * @param result_code destination for advertisement result
 * @return 0 on success, <0 on error
 */
int chip_advertise_boot_type(void) {
    return 0;
}

/**
 * @brief reset UniPro before signalling readiness to boot firmware to switch
 */
void chip_reset_before_ready(void) {
    tsb_reset_before_ready();
}
