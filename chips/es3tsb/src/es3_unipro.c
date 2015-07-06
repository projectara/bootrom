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

struct cport {
    uint8_t *tx_buf;                /* TX region for this CPort */
    uint8_t *rx_buf;                /* RX region for this CPort */
    uint16_t cportid;
};

#define CPORT_BUF_SIZE            (0x2000U)
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
}

static struct cport cporttable[] = {
    DECLARE_CPORT(0),  DECLARE_CPORT(1),  DECLARE_CPORT(2),  DECLARE_CPORT(3),
};
#define CPORT_MAX  (sizeof(cporttable)/sizeof(struct cport))

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

/**
 * @brief send data down a CPort
 * @param cportid cport to send down
 * @param buf data buffer
 * @param len size of data to send
 * @param 0 on success, <0 on error
 */
int chip_unipro_send(unsigned int cportid, const void *buf, size_t len) {
    unsigned int i;
    struct cport *cport;
    char *data = (char*)buf;

    if (cportid >= CPORT_MAX || len > CPORT_BUF_SIZE) {
        return -1;
    }

    cport = cport_handle(cportid);
    if (!cport) {
        return -1;
    }

    /*
     * Data payload
     */
    for (i = 0; i < len; i++) {
        putreg8(data[i], &cport->tx_buf[i]);
    }

    /* Hit EOM */
    putreg8(1, CPORT_EOM_BIT(cport));

    return 0;
}

static void chip_unipro_restart_rx(struct cport *cport) {
    unsigned int cportid = cport->cportid;

    unipro_write(AHM_ADDRESS_00 + (cportid << 2), (uint32_t)cport->rx_buf);
    unipro_write(REG_RX_PAUSE_SIZE_00 + (cportid << 2),
                 RX_PAUSE_RESTART | CPORT_RX_BUF_SIZE);
}

int chip_unipro_receive(unsigned int cportid, unipro_rx_handler handler) {
    uint32_t bytes_received;
    struct cport *cport;

    uint32_t eom_nom_bit;
    uint32_t eom_err_bit;
    uint32_t eot_bit;

    uint32_t eom;
    uint32_t eot;

    cport = cport_handle(cportid);
    if (!cport) {
        return -1;
    }

    while(1) {
        eom = unipro_read(AHM_RX_EOM_INT_BEF_0);
        eot = unipro_read(AHM_RX_EOT_INT_BEF_0);

        eom_nom_bit = (0x01 << (cportid << 1));
        eom_err_bit = (0x10 << (cportid << 1));
        eot_bit = (1 << cportid);

        if ((eom & eom_err_bit) != 0) {
            dbgprint("UniPro RX error\r\n");
            return -1;
        }
        if ((eot & eot_bit) != 0) {
            dbgprint("data received exceeded max length\r\n");
            return -1;
        }
        if ((eom & eom_nom_bit) != 0) {
            bytes_received = unipro_read(CPB_RX_TRANSFERRED_DATA_SIZE_00 +
                                         (cportid << 2));
            dbgprint("cport ");
            dbgprinthex8(cportid);
            dbgprint(" received ");
            dbgprinthex32(bytes_received);
            dbgprint(" bytes of data\r\n");
            unipro_write(AHM_RX_EOM_INT_BEF_0, eom_nom_bit);

            if (handler != NULL) {
                if(0 != handler(cportid,
                                cport->rx_buf,
                                bytes_received)) {
                    dbgprint("RX handler returned error\r\n");
                    return -1;
                }
            }
            chip_unipro_restart_rx(cport);
            return 0;
        }
    }
    return 0;
}

void chip_unipro_init_cport(int16_t cportid) {
    struct cport *cport;

    if (cportid >= CPORT_MAX) {
        return;
    }

    cport = cport_handle(cportid);
    if (!cport) {
        return;
    }

    chip_unipro_restart_rx(cport);
}

void chip_unipro_init(void) {
    chip_unipro_init_cport(CONTROL_CPORT);
}
