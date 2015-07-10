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
#include "tsb_scm.h"
#include "tsb_unipro.h"
#include "debug.h"
#include "es2_unipro.h"
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

/*
 * "Map" constants for M-PHY fixups.
 */
#define TSB_MPHY_MAP (0x7F)
    #define TSB_MPHY_MAP_TSB_REGISTER_1 (0x01)
    #define TSB_MPHY_MAP_NORMAL         (0x00)
    #define TSB_MPHY_MAP_TSB_REGISTER_2 (0x81)

/*
 * This specifies an M-PHY "fixup"; i.e., a value that must be set to
 * a DME attribute while the link is still in PWM-G1, before
 * transitioning to HS link power modes.
 *
 * Use tsb_mphy_r1_fixup_is_magic() to test if a register 1 map fixup
 * must have its value drawn from M-PHY trim values or magic debug
 * registers (switch ports and bridges handle this case differently).
 */
struct tsb_mphy_fixup {
    uint16_t attrid;
    uint16_t select_index;
    uint32_t value;

#define TSB_MPHY_FIXUP_LAST     0x1
#define TSB_MPHY_FIXUP_MAGIC_R1 0x2
    uint32_t flags;
};

/*
 * Use tsb_mphy_fixup_is_last() to test if a fixup is the last one in
 * each of these array.
 */
extern const struct tsb_mphy_fixup tsb_register_1_map_mphy_fixups[];
extern const struct tsb_mphy_fixup tsb_register_2_map_mphy_fixups[];

static inline int tsb_mphy_fixup_is_last(const struct tsb_mphy_fixup *fu) {
    return !!(fu->flags & TSB_MPHY_FIXUP_LAST);
}

static inline int tsb_mphy_r1_fixup_is_magic(const struct tsb_mphy_fixup *fu) {
    return !!(fu->flags & TSB_MPHY_FIXUP_MAGIC_R1);
}

#define __TSB_MPHY_FIXUP(a, s, v, f)                                    \
    { .attrid = (a), .select_index = (s), .value = (v), .flags = (f) }
#define TSB_MPHY_FIXUP(a, s, v)                                         \
    __TSB_MPHY_FIXUP((a), (s), (v), 0)
#define TSB_MPHY_LAST_FIXUP(a, s, v)                                    \
    __TSB_MPHY_FIXUP((a), (s), (v), TSB_MPHY_FIXUP_LAST)
#define TSB_MPHY_MAGIC_R1_FIXUP()                                       \
    __TSB_MPHY_FIXUP(0, 0, 0, TSB_MPHY_FIXUP_MAGIC_R1)

const struct tsb_mphy_fixup tsb_register_1_map_mphy_fixups[] = {
    TSB_MPHY_MAGIC_R1_FIXUP(),

    TSB_MPHY_FIXUP(0x8004, 0, 0xCA),
    TSB_MPHY_FIXUP(0x8015, 0, 0x01),
    TSB_MPHY_FIXUP(0x8022, 0, 0x44),
    TSB_MPHY_FIXUP(0x8023, 0, 0x42),
    TSB_MPHY_FIXUP(0x80A2, 0, 0x00),
    TSB_MPHY_FIXUP(0x80AA, 0, 0xA8),
    TSB_MPHY_FIXUP(0x80BA, 0, 0x20),

    TSB_MPHY_FIXUP(0x80A2, 1, 0x00),
    TSB_MPHY_FIXUP(0x80AA, 1, 0xA8),
    TSB_MPHY_FIXUP(0x80BA, 1, 0x20),

    TSB_MPHY_FIXUP(0x8094, 4, 0x09),
    TSB_MPHY_FIXUP(0x809A, 4, 0x06),
    TSB_MPHY_FIXUP(0x809B, 4, 0x03),
    TSB_MPHY_FIXUP(0x809C, 4, 0x00),
    TSB_MPHY_FIXUP(0x80AA, 4, 0x0F),
    TSB_MPHY_FIXUP(0x80B4, 4, 0x50),
    TSB_MPHY_FIXUP(0x80B6, 4, 0x82),
    TSB_MPHY_FIXUP(0x80B7, 4, 0x01),

    TSB_MPHY_FIXUP(0x8094, 5, 0x09),
    TSB_MPHY_FIXUP(0x809A, 5, 0x06),
    TSB_MPHY_FIXUP(0x809B, 5, 0x03),
    TSB_MPHY_FIXUP(0x809C, 5, 0x00),
    TSB_MPHY_FIXUP(0x80AA, 5, 0x0F),
    TSB_MPHY_FIXUP(0x80B4, 5, 0x50),
    TSB_MPHY_FIXUP(0x80B6, 5, 0x82),
    TSB_MPHY_FIXUP(0x80B7, 5, 0x01),

    TSB_MPHY_LAST_FIXUP(0x8000, 0, 0x01),
};

const struct tsb_mphy_fixup tsb_register_2_map_mphy_fixups[] = {
    TSB_MPHY_FIXUP(0x8000, 0, 0x02),

    TSB_MPHY_FIXUP(0x8080, 0, 0x20),
    TSB_MPHY_FIXUP(0x8081, 0, 0x03),

    TSB_MPHY_FIXUP(0x8080, 1, 0x20),
    TSB_MPHY_FIXUP(0x8081, 1, 0x03),

    TSB_MPHY_FIXUP(0x8082, 4, 0x3F),
    TSB_MPHY_FIXUP(0x8084, 4, 0x10),
    TSB_MPHY_FIXUP(0x8086, 4, 0x10),
    TSB_MPHY_FIXUP(0x8087, 4, 0x01),
    TSB_MPHY_FIXUP(0x8088, 4, 0x10),
    TSB_MPHY_FIXUP(0x808D, 4, 0x0B),
    TSB_MPHY_FIXUP(0x808E, 4, 0x00),
    TSB_MPHY_FIXUP(0x8094, 4, 0x00),
    TSB_MPHY_FIXUP(0x8096, 4, 0x00),
    TSB_MPHY_FIXUP(0x8098, 4, 0x08),
    TSB_MPHY_FIXUP(0x8099, 4, 0x50),

    TSB_MPHY_FIXUP(0x8082, 5, 0x3F),
    TSB_MPHY_FIXUP(0x8084, 5, 0x10),
    TSB_MPHY_FIXUP(0x8086, 5, 0x10),
    TSB_MPHY_FIXUP(0x8087, 5, 0x01),
    TSB_MPHY_FIXUP(0x8088, 5, 0x10),
    TSB_MPHY_FIXUP(0x808D, 5, 0x0B),
    TSB_MPHY_FIXUP(0x808E, 5, 0x00),
    TSB_MPHY_FIXUP(0x8094, 5, 0x00),
    TSB_MPHY_FIXUP(0x8096, 5, 0x00),
    TSB_MPHY_FIXUP(0x8098, 5, 0x08),
    TSB_MPHY_LAST_FIXUP(0x8099, 5, 0x50),
};

static int es2_fixup_mphy(void)
{
    uint32_t debug_0720 = tsb_get_debug_reg(0x0720);
    uint32_t urc;
    const struct tsb_mphy_fixup *fu;

    /*
     * Apply the "register 2" map fixups.
     */
    unipro_attr_local_write(TSB_MPHY_MAP, TSB_MPHY_MAP_TSB_REGISTER_2, 0,
                            &urc);
    if (urc) {
        dbgprint((char*)__func__);
        dbgprintx32(": failed to switch to register 2 map:", urc, "\r\n");
        return urc;
    }
    fu = tsb_register_2_map_mphy_fixups;
    do {
        unipro_attr_local_write(fu->attrid, fu->value, fu->select_index,
                                &urc);
        if (urc) {
            dbgprint((char*)__func__);
            dbgprintx32(": failed to switch to register 2 map:", urc, "\r\n");
            return urc;
        }
    } while (!tsb_mphy_fixup_is_last(fu++));

    /*
     * Switch to "normal" map.
     */
    unipro_attr_local_write(TSB_MPHY_MAP, TSB_MPHY_MAP_NORMAL, 0,
                            &urc);
    if (urc) {
        dbgprint((char*)__func__);
        dbgprintx32(": failed to switch to normal map: ", urc, "\r\n");
        return urc;
    }

    /*
     * Apply the "register 1" map fixups.
     */
    unipro_attr_local_write(TSB_MPHY_MAP, TSB_MPHY_MAP_TSB_REGISTER_1, 0,
                            &urc);
    if (urc) {
        dbgprint((char*)__func__);
        dbgprintx32(": failed to switch to register 1 map: ", urc, "\r\n");
        return urc;
    }
    fu = tsb_register_1_map_mphy_fixups;
    do {
        if (tsb_mphy_r1_fixup_is_magic(fu)) {
            /*
             * The magic R1 fixups come from the mysterious and solemn
             * debug register 0x0720.
             * */
            unipro_attr_local_write(0x8002, (debug_0720 >> 1) & 0x1f, 0, &urc);
        } else {
            unipro_attr_local_write(fu->attrid, fu->value, fu->select_index,
                                    &urc);
        }
        if (urc) {
            dbgprint((char*)__func__);
            dbgprintx32(": failed to switch to register 1 map: ", urc, "\r\n");
            return urc;
        }
    } while (!tsb_mphy_fixup_is_last(fu++));

    /*
     * Switch to "normal" map.
     */
    unipro_attr_local_write(TSB_MPHY_MAP, TSB_MPHY_MAP_NORMAL, 0,
                            &urc);
    if (urc) {
        dbgprint((char*)__func__);
        dbgprintx32(": failed to switch to normal map: ", urc, "\r\n");
        return urc;
    }

    return 0;
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
    if(write) {
        unipro_write(A2D_ATTRACS_DATA_CTRL_00, *val);
    }

    /* Start the access */
    unipro_write(A2D_ATTRACS_MSTR_CTRL,
                 REG_ATTRACS_CNT(1) | REG_ATTRACS_UPD);

    while(!unipro_read(A2D_ATTRACS_INT_BEF))
        ;

    /* Clear status bit */
    unipro_write(A2D_ATTRACS_INT_BEF, 0x1);

    if(result_code) {
        *result_code = unipro_read(A2D_ATTRACS_STS_00);
    }

    if(!write) {
        *val = unipro_read(A2D_ATTRACS_DATA_STS_00);
    }

    return 0;
}

void configure_transfer_mode(int mode) {
  switch (mode) {
  case TRANSFER_MODE:
    unipro_write(AHM_MODE_CTRL_0, TRANSFER_MODE_2_CTRL_0);
    break;
  default:
    dbgprintx32("Unsupported transfer mode: ", mode, "\r\n");
    break;
  }
}

void chip_unipro_init(void) {
    if (es2_fixup_mphy()) {
        dbgprint("Failed to apply M-PHY fixups (results in link instability at HS-G1).\r\n");
    }

    configure_transfer_mode(TRANSFER_MODE);

    unipro_write(UNIPRO_INT_EN, 0);
    chip_unipro_init_cport(CONTROL_CPORT);
    unipro_write(UNIPRO_INT_EN, 1);

    dbgprint("Unipro enabled!\r\n");
}

int chip_unipro_attr_read(uint16_t attr, uint32_t *val, uint16_t selector,
                          int peer, uint32_t *result_code) {
    return unipro_attr_access(attr, val, selector, peer, 0, result_code);
}

int chip_unipro_attr_write(uint16_t attr, uint32_t val, uint16_t selector,
                           int peer, uint32_t *result_code) {
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
