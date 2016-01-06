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
#include "chipapi.h"
#include "tsb_unipro.h"
#include "debug.h"
#include "data_loading.h"
#include "greybus.h"


/**
 * @brief perform a DME access
 * @param attr attribute to access
 * @param val pointer to value to either read or write
 * @param peer 0 for local access, 1 for peer
 * @param write 0 for read, 1 for write
 * @param result_code UniPro return code, optional
 */
static int unipro_attr_access(uint16_t attr,
                              uint32_t *val,
                              uint16_t selector,
                              int peer,
                              int write) {
    uint32_t rc = 0;

    uint32_t ctrl = (REG_ATTRACS_CTRL_PEERENA(peer) |
                     REG_ATTRACS_CTRL_SELECT(selector) |
                     REG_ATTRACS_CTRL_WRITE(write) |
                     attr);

    tsb_unipro_write(A2D_ATTRACS_CTRL_00, ctrl);
    if (write) {
        tsb_unipro_write(A2D_ATTRACS_DATA_CTRL_00, *val);
    }

    /* Start the access */
    tsb_unipro_write(A2D_ATTRACS_MSTR_CTRL,
                      REG_ATTRACS_CNT(1) | REG_ATTRACS_UPD);

    while (!tsb_unipro_read(A2D_ATTRACS_INT_BEF))
        ;

    /* Clear status bit */
    tsb_unipro_write(A2D_ATTRACS_INT_BEF, 0x1);

    rc = tsb_unipro_read(A2D_ATTRACS_STS_00);

    if (!write) {
        *val = tsb_unipro_read(A2D_ATTRACS_DATA_STS_00);
    }

    return rc;
}


int chip_unipro_attr_read(uint16_t attr,
                          uint32_t *val,
                          uint16_t selector,
                          int peer)
{
    return unipro_attr_access(attr, val, selector, peer, 0);
}


int chip_unipro_attr_write(uint16_t attr,
                           uint32_t val,
                           uint16_t selector,
                           int peer)
{
#if (defined _DME_LOGGING) && (defined _DEBUGMSGS)
    /* Log all DME writes except those related to UniPro boot handshake. */
    if ((attr != ARA_MAILBOX) && (attr != ARA_INTERRUPTSTATUS) &&
        (attr != ARA_INTERRUPTSTATUS_MAILBOX) && (attr != ARA_MBOX_ACK_ATTR))
    {
        dbgprintx16("ID=", attr, NULL);
        dbgprintx32(", Val=", val, "\n");
    }
#endif
    return unipro_attr_access(attr, &val, selector, peer, 1);
}

void chip_unipro_init(void) {
    tsb_reset_all_cports();
    dbgprint("Unipro enabled\n");
}

int chip_unipro_init_cport(uint32_t cportid) {
    return tsb_unipro_init_cport(cportid);
}

int chip_unipro_recv_cport(uint32_t *cportid) {
    return tsb_unipro_recv_cport(cportid);
}

/* TA-11 Operate UniPro function with One CPORT and transfer mode */

void chip_reset_before_jump(void) {
    tsb_reset_before_jump();
}
