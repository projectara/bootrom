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
#include "bootrom.h"

uint32_t br_errno;

#ifdef _SIMULATION
int chip_enter_hibern8_client(void);
int chip_exit_hibern8_client(void);

void resume_point(void) {
    chip_init();

    dbginit();

    dbgprint("Resumed from standby\n");
    chip_exit_hibern8_client();

    /* handshake with test controller to indicate success */
    chip_signal_boot_status(0);
    while(1);
}

/* use all 4 GPIOs 3, 4, 5, 23 as wakeup source */
#define TEST_WAKEUPSRC 0x1111
int enter_standby(void) {
    int (*enter_standby_func)(void);
    int status;
    communication_area *p = (communication_area *)&_communication_area;

    p->resume_data.jtag_disabled = 1;
    p->resume_data.resume_address = (uint32_t)resume_point;
    p->resume_data.resume_address_complement = ~(uint32_t)resume_point;

    chip_enter_hibern8_client();

    putreg32(TEST_WAKEUPSRC, WAKEUPSRC);

    enter_standby_func = get_shared_function(SHARED_FUNCTION_ENTER_STANDBY);

    status = enter_standby_func();
    return status;
}
#endif

/**
 * @brief Bootloader "C" entry point
 *
 * @param none
 *
 * @returns Nothing. Will launch/restart image if successful, halt if not.
 */
void bootrom_main(void) {

    chip_init();

    dbginit();

    dbgprint("Hello world from s3fw\n");

#ifdef _SIMULATION
    /* Handshake with the controller, indicating trying to enter standby */
    chip_signal_boot_status(0);
    enter_standby();
#endif
    /* Our work is done */
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
