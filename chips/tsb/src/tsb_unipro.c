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

struct cport {
    uint8_t *tx_buf;                /* TX region for this CPort */
    uint8_t *rx_buf;                /* RX region for this CPort */
    uint16_t cportid;
    int connected;
};

/* TBD CPORT MAX */
#define CPORT_MAX                   (2)

#define CPORT_BUF_SIZE            (0x10000U)
#define CPORT_RX_BUF_BASE         (0x20000000U)
#define CPORT_RX_BUF_SIZE         (CPORT_BUF_SIZE)
#define CPORT_RX_BUF(cport)       (void*)(CPORT_RX_BUF_BASE + \
                                      (CPORT_RX_BUF_SIZE * cport))
#define CPORT_TX_BUF_BASE         (0x50000000U)
#define CPORT_TX_BUF_SIZE         (0x20000U)
#define CPORT_TX_BUF(cport)       (uint8_t*)(CPORT_TX_BUF_BASE + \
                                      (CPORT_TX_BUF_SIZE * cport))
#define CPORT_EOM_BIT(cport)      (cport->tx_buf + (CPORT_TX_BUF_SIZE - 1))

#define DECLARE_CPORT(id) {            \
    .tx_buf      = CPORT_TX_BUF(id),   \
    .rx_buf      = CPORT_RX_BUF(id),   \
    .cportid     = id,                 \
    .connected   = 0,                  \
}

static struct cport cporttable[] = {
    DECLARE_CPORT(0),  DECLARE_CPORT(1),  DECLARE_CPORT(2),  DECLARE_CPORT(3),
    DECLARE_CPORT(4),  DECLARE_CPORT(5),  DECLARE_CPORT(6),  DECLARE_CPORT(7),
    DECLARE_CPORT(8),  DECLARE_CPORT(9),  DECLARE_CPORT(10), DECLARE_CPORT(11),
    DECLARE_CPORT(12), DECLARE_CPORT(13), DECLARE_CPORT(14), DECLARE_CPORT(15),
    DECLARE_CPORT(16), DECLARE_CPORT(17), DECLARE_CPORT(18), DECLARE_CPORT(19),
    DECLARE_CPORT(20), DECLARE_CPORT(21), DECLARE_CPORT(22), DECLARE_CPORT(23),
    DECLARE_CPORT(24), DECLARE_CPORT(25), DECLARE_CPORT(26), DECLARE_CPORT(27),
    DECLARE_CPORT(28), DECLARE_CPORT(29), DECLARE_CPORT(30), DECLARE_CPORT(31),
};

static inline struct cport *cport_handle(uint16_t cportid) {
    if (cportid >= CPORT_MAX) {
        return NULL;
    } else {
        return &cporttable[cportid];
    }
}

static uint32_t unipro_read(uint32_t offset) {
    return getreg32((volatile unsigned int*)(AIO_UNIPRO_BASE + offset));
}

static void unipro_write(uint32_t offset, uint32_t v) {
    putreg32(v, (volatile unsigned int*)(AIO_UNIPRO_BASE + offset));
}

/**
 * @brief perform a DME access
 * @param attr attribute to access
 * @param val pointer to value to either read or write
 * @param peer 0 for local access, 1 for peer
 * @param write 0 for read, 1 for write
 * @param result_code unipro return code, optional
 */
static int unipro_attr_access(uint16_t attr,
                              uint32_t *val,
                              uint16_t selector,
                              int peer,
                              int write,
                              uint32_t *result_code) {

    uint32_t ctrl = (REG_ATTRACS_CTRL_PEERENA(peer) |
                     REG_ATTRACS_CTRL_SELECT(selector) |
                     REG_ATTRACS_CTRL_WRITE(write) |
                     attr);

    unipro_write(A2D_ATTRACS_CTRL_00, ctrl);
    if (write) {
        unipro_write(A2D_ATTRACS_DATA_CTRL_00, *val);
    }

    /* Start the access */
    unipro_write(A2D_ATTRACS_MSTR_CTRL,
                 REG_ATTRACS_CNT(1) | REG_ATTRACS_UPD);

    while (!unipro_read(A2D_ATTRACS_INT_BEF))
        ;

    /* Clear status bit */
    unipro_write(A2D_ATTRACS_INT_BEF, 0x1);

    if (result_code) {
        *result_code = unipro_read(A2D_ATTRACS_STS_00);
    }

    if (!write) {
        *val = unipro_read(A2D_ATTRACS_DATA_STS_00);
    }

    return 0;
}


int chip_unipro_attr_read(uint16_t attr,
                          uint32_t *val,
                          uint16_t selector,
                          int peer,
                          uint32_t *result_code)
{
    return unipro_attr_access(attr, val, selector, peer, 0, result_code);
}


int chip_unipro_attr_write(uint16_t attr,
                           uint32_t val,
                           uint16_t selector,
                           int peer,
                           uint32_t *result_code)
{
    return unipro_attr_access(attr, &val, selector, peer, 1, result_code);
}

void chip_unipro_init(void) {
}
