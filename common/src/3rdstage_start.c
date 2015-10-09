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
#include "utils.h"

void tsb_get_cms(uint8_t * buf, uint32_t size);
void tsb_enable_ims_access(void);
void tsb_enable_cms_access(void);

void check_ims_cms_access(void) {
    int i;
    unsigned char ims[TSB_ISAA_NUM_IMS_BYTES];
    unsigned char cms[TSB_ISAA_NUM_CMS_BYTES];
    tsb_get_ims(ims, TSB_ISAA_NUM_IMS_BYTES);
    if (is_constant_fill(ims, TSB_ISAA_NUM_IMS_BYTES, 0)) {
        dbgprint("IMS reads all zero\n");
    } else {
        dbgprint("IMS reads NOT all zero\n");
        dbgprint("   ");
        for (i = 0; i < TSB_ISAA_NUM_IMS_BYTES; i++) {
            dbgprinthex8(ims[i]);
        }
        dbgprint("\n");
    }

    tsb_get_cms(cms, TSB_ISAA_NUM_CMS_BYTES);
    if (is_constant_fill(cms, TSB_ISAA_NUM_CMS_BYTES, 0)) {
        dbgprint("CMS reads all zero\n");
    } else {
        dbgprint("CMS reads NOT all zero\n");
        dbgprint("   ");
        for (i = 0; i < TSB_ISAA_NUM_CMS_BYTES; i++) {
            dbgprinthex8(cms[i]);
        }
        dbgprint("\n");
    }

    dbgprint("Try to enable access to IMS/CMS\n");
    tsb_enable_ims_access();
    tsb_enable_cms_access();

    tsb_get_ims(ims, TSB_ISAA_NUM_IMS_BYTES);
    if (is_constant_fill(ims, TSB_ISAA_NUM_IMS_BYTES, 0)) {
        dbgprint("IMS reads all zero after trying to enable access\n");
    } else {
        dbgprint("IMS reads NOT all zero after trying to enable access\n");
        dbgprint("   ");
        for (i = 0; i < TSB_ISAA_NUM_IMS_BYTES; i++) {
            dbgprinthex8(ims[i]);
        }
        dbgprint("\n");
    }

    tsb_get_cms(cms, TSB_ISAA_NUM_CMS_BYTES);
    if (is_constant_fill(cms, TSB_ISAA_NUM_CMS_BYTES, 0)) {
        dbgprint("CMS reads all zero after trying to enable access\n");
    } else {
        dbgprint("CMS reads NOT all zero after trying to enable access\n");
        dbgprint("   ");
        for (i = 0; i < TSB_ISAA_NUM_CMS_BYTES; i++) {
            dbgprinthex8(cms[i]);
        }
        dbgprint("\n");
    }
}

#ifdef _STANDBY_TEST
int chip_enter_hibern8_client(void);
int chip_exit_hibern8_client(void);

void resume_sequence_in_workram(void);
void resume_point(void) {
    /**
     * NOTE:
     * this experiment code runs resume_point on the stack of boot ROM
     * real code needs to save/restore its own stack pointer
     */
    resume_sequence_in_workram();

    check_ims_cms_access();
    /* handshake with test controller to indicate success */
    chip_handshake_boot_status(0);
    while(1);
}

int standby_sequence(void);
int enter_standby(void) {
    communication_area *p = (communication_area *)&_communication_area;

    p->resume_data.jtag_disabled = 1;
    p->resume_data.resume_address = (uint32_t)resume_point;
    p->resume_data.resume_address_complement = ~(uint32_t)resume_point;

    return standby_sequence();
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

    init_last_error();

    dbgprint("Hello world from s3fw\n");

    check_ims_cms_access();

#ifdef _HANDSHAKE
    /* Handshake with the controller, indicating trying to enter standby */
    chip_handshake_boot_status(0);
#ifdef _STANDBY_TEST
    enter_standby();
#endif
#endif
    /* Our work is done */
    while(1);
}
