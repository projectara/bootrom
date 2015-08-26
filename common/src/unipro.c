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
#include <string.h>
#include <errno.h>
#include "bootrom.h"
#include "chipapi.h"
#include "chipdef.h"
#include "debug.h"
#include "data_loading.h"
#include "unipro.h"
#include "greybus.h"
#include "utils.h"

/**
 * @brief Synchronously read from our local mailbox.
 * @return 0 on success, <0 on error
 */
int read_mailbox(uint32_t *val, uint32_t *result_code) {
    int rc;
    uint32_t result = 0, tempval = TSB_MAIL_RESET;

    if (!val) {
        return -EINVAL;
    }

    do {
        rc = chip_unipro_attr_read(TSB_MAILBOX, &tempval, 0, ATTR_LOCAL,
                                   &result);
    } while (!rc && tempval == TSB_MAIL_RESET);
    if (rc) {
        return rc;
    }

    if (result_code) {
        *result_code = result;
    }
    *val = tempval;

    return 0;
}

/**
 * @brief Acknowledge that we've read our local mailbox, clearing it.
 * @return 0 on success, <0 on error
 */
int ack_mailbox(void) {
    return chip_unipro_attr_write(TSB_MAILBOX, TSB_MAIL_RESET, 0, ATTR_LOCAL,
                                  NULL);
}

/**
 * @brief Synchronously write to the peer mailbox, polling for it to be cleared
 * once we've written it.
 * @return 0 on success, <0 on error
 */
int write_mailbox(uint32_t val, uint32_t *result_code) {
    int rc;
    uint32_t result = 0, irq_status = 0;

    rc = chip_unipro_attr_write(TSB_MAILBOX, val, 0, ATTR_PEER, &result);
    if (rc) {
        return rc;
    }
    /**
     * Poll the interrupt-assert line on the switch until we know the SVC has
     * picked up our mail.
     */
    do {
        rc = chip_unipro_attr_read(TSB_INTERRUPTSTATUS, &irq_status, 0,
                                   ATTR_PEER, NULL);
    } while (!rc && (irq_status & TSB_INTERRUPTSTATUS_MAILBOX));
    if (rc) {
        return rc;
    }
    /**
     * Now poll the switch mailbox until it's cleared, indicating the SVC has
     * given us the go-ahead.
     */
    do {
        rc = chip_unipro_attr_read(TSB_MAILBOX, &val, 0, ATTR_PEER, NULL);
    } while (!rc && val != TSB_MAIL_RESET);
    if (rc) {
        return rc;
    }

    if (result_code) {
        *result_code = result;
    }
    return 0;
}

/**
 * Common code for advertising readiness to boot firmware to the switch
 */
int advertise_ready(void) {
    int rc;

    chip_reset_before_ready();

    /**
     * Write that we're a ready non-AP module to the switch's mailbox
     * attribute.
     */
    rc = write_mailbox(TSB_MAIL_READY_OTHER, NULL);
    if (rc) {
        return rc;
    }

    dbgprint("Module ready advertised.\r\n");
    return 0;
}
