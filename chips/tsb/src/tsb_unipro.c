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
#include "unipro.h"
#include "tsb_unipro.h"
#include "debug.h"

struct cport cporttable[4] = {
    DECLARE_CPORT(0),  DECLARE_CPORT(1),  DECLARE_CPORT(2),  DECLARE_CPORT(3),
};

#define CPORT_SW_RESET_BITS 3
static int tsb_unipro_reset_cport(uint32_t cportid) {
    int rc;
    uint32_t result;
    uint32_t tx_reset_offset, rx_reset_offset;

    if (cportid >= CPORT_MAX) {
        return -EINVAL;
    }

    tx_reset_offset = TX_SW_RESET_00 + (cportid << 2);
    rx_reset_offset = RX_SW_RESET_00 + (cportid << 2);

    putreg32(CPORT_SW_RESET_BITS,
             (volatile unsigned int*)(AIO_UNIPRO_BASE + tx_reset_offset));

    rc = chip_unipro_attr_write(T_CONNECTIONSTATE, 0, cportid, 0, &result);
    if (rc || result) {
        dbgprint("error resetting T_CONNECTIONSTATE\r\n");
        return -EIO;
    }

    rc = chip_unipro_attr_write(T_LOCALBUFFERSPACE, 0, cportid, 0, &result);
    if (rc || result) {
        dbgprint("error resetting T_LOCALBUFFERSPACE\r\n");
        return -EIO;
    }

    rc = chip_unipro_attr_write(T_PEERBUFFERSPACE, 0, cportid, 0, &result);
    if (rc || result) {
        dbgprint("error resetting T_PEERBUFFERSPACE\r\n");
        return -EIO;
    }

    rc = chip_unipro_attr_write(T_CREDITSTOSEND, 0, cportid, 0, &result);
    if (rc || result) {
        dbgprint("error resetting T_CREDITSTOSEND\r\n");
        return -EIO;
    }

    putreg32(CPORT_SW_RESET_BITS,
             (volatile unsigned int*)(AIO_UNIPRO_BASE + rx_reset_offset));
    putreg32(0, (volatile unsigned int*)(AIO_UNIPRO_BASE + tx_reset_offset));
    putreg32(0, (volatile unsigned int*)(AIO_UNIPRO_BASE + rx_reset_offset));
    return 0;
}

int tsb_reset_all_cports(void) {
    uint32_t i;
    int rc;

    for (i = 0; i < CPORT_MAX; i++) {
        rc = tsb_unipro_reset_cport(i);
        if (rc) {
            dbgprintx32("Failed to reset cport 0x", i, "\r\n");
            return rc;
        }
    }

    return 0;
}

/**
 * @brief Initialize a specific CPort
 */
int tsb_unipro_init_cport(uint32_t cportid) {
    struct cport *cport;
    int rc;
    uint32_t mail = 0;

    if (cportid >= CPORT_MAX) {
        return -EINVAL;
    }

    cport = cport_handle(cportid);
    if (!cport) {
        return -EINVAL;
    }

    rc = read_mailbox(&mail);
    if (rc) {
        return rc;
    }
    if (mail != cportid + 1) {
        return -ENOSYS;
    }

    tsb_unipro_restart_rx(cport);
    tsb_enable_e2efc(cportid);

    return ack_mailbox();
}

/**
 * @brief Receive a dynamically-assigned CPort identifier
 */
int tsb_unipro_recv_cport(uint32_t *cportid) {
    struct cport *cport;
    int rc;
    uint32_t cport_recv = 0;

    rc = read_mailbox(&cport_recv);
    if (rc) {
        return rc;
    }
    *cportid = --cport_recv;

    if (cport_recv >= CPORT_MAX) {
        return -EINVAL;
    }

    cport = cport_handle(cport_recv);
    if (!cport) {
        return -EINVAL;
    }

    tsb_unipro_restart_rx(cport);
    tsb_enable_e2efc(*cportid);

    return ack_mailbox();
}

uint32_t tsb_unipro_read(uint32_t offset) {
    return getreg32((volatile unsigned int*)(AIO_UNIPRO_BASE + offset));
}

void tsb_unipro_write(uint32_t offset, uint32_t v) {
    putreg32(v, (volatile unsigned int*)(AIO_UNIPRO_BASE + offset));
}

void tsb_unipro_restart_rx(struct cport *cport) {
    unsigned int cportid = cport->cportid;

    tsb_unipro_write(AHM_ADDRESS_00 + (cportid << 2), (uint32_t)cport->rx_buf);
    tsb_unipro_write(REG_RX_PAUSE_SIZE_00 + (cportid << 2),
                 RX_PAUSE_RESTART | CPORT_RX_BUF_SIZE);
}

/**
 * @brief Enable E2EFC on a specific CPort
 * @param cportid cport on which to enable End-to-End Flow Control
 */
void tsb_enable_e2efc(uint16_t cportid) {
    uint32_t e2efc;
    if (cportid < 32) {
        e2efc = tsb_unipro_read(CPB_RX_E2EFC_EN_0);
        e2efc |= (1 << cportid);
        tsb_unipro_write(CPB_RX_E2EFC_EN_0, e2efc);
    } else if (cportid < CPORT_MAX) {
        e2efc = tsb_unipro_read(CPB_RX_E2EFC_EN_1);
        e2efc |= (1 << (cportid - 32));
        tsb_unipro_write(CPB_RX_E2EFC_EN_1, e2efc);
    }
}

/**
 * @brief Disable E2EFC on all CPorts
 */
void tsb_disable_all_e2efc(void) {
    tsb_unipro_write(CPB_RX_E2EFC_EN_0, 0);
    tsb_unipro_write(CPB_RX_E2EFC_EN_1, 0);
}

/**
 * @brief Chip-common parts of resetting before signalling readiness.
 */
void tsb_reset_before_ready(void) {
    tsb_disable_all_e2efc();
}
