/*
 * Copyright (c) 2014 Google Inc.
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

#ifndef __ARCH_ARM_SRC_TSB_TSB_UNIPRO_H
#define __ARCH_ARM_SRC_TSB_TSB_UNIPRO_H

#include "chip.h"
#include "unipro.h"

#include "tsb_unipro_hw.h"
#include "tsb_dme.h"

/**
 * Definitions for the TSB UniPro handling FW
 */
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

/*
 * Common UniPro structures and functions
 */

#define DECLARE_CPORT(id) {            \
    .tx_buf      = CPORT_TX_BUF(id),   \
    .rx_buf      = CPORT_RX_BUF(id),   \
    .cportid     = id,                 \
}

struct cport {
    uint8_t *tx_buf;                /* TX region for this CPort */
    uint8_t *rx_buf;                /* RX region for this CPort */
    uint16_t cportid;
};

extern struct cport cporttable[4];
#define CPORT_MAX  (sizeof(cporttable)/sizeof(struct cport))

static inline struct cport *cport_handle(uint16_t cportid) {
    if (cportid >= CPORT_MAX) {
        return NULL;
    } else {
        return &cporttable[cportid];
    }
}

int tsb_reset_all_cports(void);

int tsb_unipro_init_cport(uint32_t cportid);
int tsb_unipro_recv_cport(uint32_t *cportid);

uint32_t tsb_unipro_read(uint32_t offset);
void tsb_unipro_write(uint32_t offset, uint32_t v);
void tsb_unipro_restart_rx(struct cport *cport);

/**
 * @brief Disable E2EFC on all CPorts
 */
void tsb_disable_all_e2efc(void);

/**
 * @brief Chip-common parts of resetting before signalling readiness.
 */
void tsb_reset_before_ready(void);

void tsb_reset_before_jump(void);

#endif /* __ARCH_ARM_SRC_TSB_TSB_UNIPRO_H */
