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

    if (result_code) {
        *result_code = tsb_unipro_read(A2D_ATTRACS_STS_00);
    }

    if (!write) {
        *val = tsb_unipro_read(A2D_ATTRACS_DATA_STS_00);
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
        eom = tsb_unipro_read(AHM_RX_EOM_INT_BEF_0);
        eot = tsb_unipro_read(AHM_RX_EOT_INT_BEF_0);

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
            bytes_received = tsb_unipro_read(CPB_RX_TRANSFERRED_DATA_SIZE_00 +
                                             (cportid << 2));
            tsb_unipro_write(AHM_RX_EOM_INT_BEF_0, eom_nom_bit);

            if (handler != NULL) {
                if(0 != handler(cportid,
                                cport->rx_buf,
                                bytes_received)) {
                    dbgprint("RX handler returned error\r\n");
                    return -1;
                }
            }
            tsb_unipro_restart_rx(cport);
            return 0;
        }
    }
    return 0;
}

void chip_unipro_init(void) {
    dbgprint("Unipro enabled!\r\n");
}

int chip_unipro_init_cport(uint32_t cportid) {
    return tsb_unipro_init_cport(cportid);
}

int chip_unipro_recv_cport(uint32_t *cportid) {
    return tsb_unipro_recv_cport(cportid);
}

/* TA-11 Operate UniPro function with One CPORT and transfer mode */
