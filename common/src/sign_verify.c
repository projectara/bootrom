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
 * Special, never-to-be-released s3fw that verifies that it runs
 */

#include <stdint.h>
#include <stdbool.h>
#include "chipapi.h"
#include "tsb_scm.h"
#include "tsb_isaa.h"
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
#include "string.h"

union large_uint {
  struct {
    uint32_t low;
    uint32_t high;
  };
  uint64_t quad;
  uint8_t buffer[8];
};

void tsb_get_ims(uint8_t * buf, uint32_t size);
void tsb_get_cms(uint8_t * buf, uint32_t size);

void check_ims_cms_access(void);


/**
 * @brief Bootloader "C" entry point
 *
 * @param none
 *
 * @returns Nothing. Will launch/restart image if successful, halt if not.
 */
void bootrom_main(void) {
    communication_area *comm_area = (communication_area *)&_communication_area;
    union large_uint eid;

    memcpy(&eid.buffer, comm_area->endpoint_unique_id, sizeof(eid.buffer));

    chip_init();

    dbginit();

    init_last_error();

    dbgprint("sign-verify running...\n");
    check_ims_cms_access();
    dbgprintx64("endpoint ID: ", eid.quad, "\n");
    dbgprint("sign-verify done\n");

    /* Our work is done */
    while(1);
}

/**
 * @brief Verify that
 *
 * @param none
 *
 * @returns Nothing. Will launch/restart image if successful, halt if not.
 */
void check_ims_cms_access(void) {
    unsigned char ims[TSB_ISAA_NUM_IMS_BYTES];
    unsigned char cms[TSB_ISAA_NUM_CMS_BYTES];
    bool zero_ims;
    bool zero_cms;

    /* Get, verify and clear the IMS and CMS values quickly */
    tsb_get_ims(ims, TSB_ISAA_NUM_IMS_BYTES);
    zero_ims = is_constant_fill(ims, TSB_ISAA_NUM_IMS_BYTES, 0);
    memset(ims, 0, sizeof(ims));

    tsb_get_cms(cms, TSB_ISAA_NUM_CMS_BYTES);
    zero_cms = is_constant_fill(cms, TSB_ISAA_NUM_CMS_BYTES, 0);
    memset(cms, 0, sizeof(cms));

    /* Now that they've been cleared, we can issue debug messages */
    dbgprint("IMS access ");
    dbgprint((tsb_get_disable_ims_access() == 0)? "en" : "dis");
    dbgprint("abled\n");
    dbgprint("IMS reads ");
    dbgprint(zero_ims? "" : "non-");
    dbgprint("zero\n");

    dbgprint("CMS access ");
    dbgprint((tsb_get_disable_cms_access() == 0)? "en" : "dis");
    dbgprint("abled\n");
    dbgprint("CMS reads ");
    dbgprint(zero_cms? "" : "non-");
    dbgprint("zero\n");
}
