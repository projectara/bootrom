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
#include "bootrom.h"
#include "chipapi.h"
#include "tsb_scm.h"
#include "tsb_unipro.h"
#include "debug.h"
#include "es2_unipro.h"
#include "greybus.h"

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
    int urc;
    const struct tsb_mphy_fixup *fu;

    /*
     * Apply the "register 2" map fixups.
     */
    urc = unipro_attr_local_write(TSB_MPHY_MAP, TSB_MPHY_MAP_TSB_REGISTER_2, 0);
    if (urc) {
        dbgprint((char*)__func__);
        dbgprintx32(": failed to switch to register 2 map:", urc, "\n");
        return urc;
    }
    fu = tsb_register_2_map_mphy_fixups;
    do {
        urc = unipro_attr_local_write(fu->attrid, fu->value, fu->select_index);
        if (urc) {
            dbgprint((char*)__func__);
            dbgprintx32(": failed to switch to register 2 map:", urc, "\n");
            return urc;
        }
    } while (!tsb_mphy_fixup_is_last(fu++));

    /*
     * Switch to "normal" map.
     */
    urc = unipro_attr_local_write(TSB_MPHY_MAP, TSB_MPHY_MAP_NORMAL, 0);
    if (urc) {
        dbgprint((char*)__func__);
        dbgprintx32(": failed to switch to normal map: ", urc, "\n");
        return urc;
    }

    /*
     * Apply the "register 1" map fixups.
     */
    urc = unipro_attr_local_write(TSB_MPHY_MAP, TSB_MPHY_MAP_TSB_REGISTER_1, 0);
    if (urc) {
        dbgprint((char*)__func__);
        dbgprintx32(": failed to switch to register 1 map: ", urc, "\n");
        return urc;
    }
    fu = tsb_register_1_map_mphy_fixups;
    do {
        if (tsb_mphy_r1_fixup_is_magic(fu)) {
            /*
             * The magic R1 fixups come from the mysterious and solemn
             * debug register 0x0720.
             * */
            urc = unipro_attr_local_write(0x8002, (debug_0720 >> 1) & 0x1f, 0);
        } else {
            urc = unipro_attr_local_write(fu->attrid, fu->value,
                                          fu->select_index);
        }
        if (urc) {
            dbgprint((char*)__func__);
            dbgprintx32(": failed to switch to register 1 map: ", urc, "\n");
            return urc;
        }
    } while (!tsb_mphy_fixup_is_last(fu++));

    /*
     * Switch to "normal" map.
     */
    urc = unipro_attr_local_write(TSB_MPHY_MAP, TSB_MPHY_MAP_NORMAL, 0);
    if (urc) {
        dbgprint((char*)__func__);
        dbgprintx32(": failed to switch to normal map: ", urc, "\n");
        return urc;
    }

    return 0;
}

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
    int rc = 0;

    if (attr == ARA_MBOX_ACK_ATTR) {
        attr = ES2_MBOX_ACK_ATTR;
    }

    uint32_t ctrl = (REG_ATTRACS_CTRL_PEERENA(peer) |
                     REG_ATTRACS_CTRL_SELECT(selector) |
                     REG_ATTRACS_CTRL_WRITE(write) |
                     attr);

    tsb_unipro_write(A2D_ATTRACS_CTRL_00, ctrl);
    if(write) {
        tsb_unipro_write(A2D_ATTRACS_DATA_CTRL_00, *val);
    }

    /* Start the access */
    tsb_unipro_write(A2D_ATTRACS_MSTR_CTRL,
                      REG_ATTRACS_CNT(1) | REG_ATTRACS_UPD);

    while(!tsb_unipro_read(A2D_ATTRACS_INT_BEF))
        ;

    /* Clear status bit */
    tsb_unipro_write(A2D_ATTRACS_INT_BEF, 0x1);

    rc = tsb_unipro_read(A2D_ATTRACS_STS_00);

    if(!write) {
        *val = tsb_unipro_read(A2D_ATTRACS_DATA_STS_00);
    }

    return rc;
}

void configure_transfer_mode(int mode) {
  switch (mode) {
  case TRANSFER_MODE:
    tsb_unipro_write(AHM_MODE_CTRL_0, TRANSFER_MODE_2_CTRL_0);
    break;
  default:
    dbgprintx32("Unsupported transfer mode: ", mode, "\n");
    break;
  }
}

void chip_unipro_init(void) {
    if (es2_fixup_mphy()) {
        dbgprint("Failed to apply M-PHY fixups (results in link instability at HS-G1).\n");
    }

    configure_transfer_mode(TRANSFER_MODE);

    tsb_unipro_write(UNIPRO_INT_EN, 0);
    tsb_unipro_write(UNIPRO_INT_EN, 1);

    tsb_reset_all_cports();
    dbgprint("Unipro enabled!\n");
}

int chip_unipro_attr_read(uint16_t attr, uint32_t *val, uint16_t selector,
                          int peer) {
    switch(attr) {
        case DME_DDBL2_VID:
            *val = ara_vid;
            return 0;
            break;
        case DME_DDBL2_PID:
            *val = ara_pid;
            return 0;
            break;
        default:
            break;
    }
    return unipro_attr_access(attr, val, selector, peer, 0);
}

int chip_unipro_attr_write(uint16_t attr, uint32_t val, uint16_t selector,
                           int peer) {
    return unipro_attr_access(attr, &val, selector, peer, 1);
}

int chip_unipro_init_cport(uint32_t cportid) {
    return tsb_unipro_init_cport(cportid);
}

int chip_unipro_recv_cport(uint32_t *cportid) {
    return tsb_unipro_recv_cport(cportid);
}

void chip_reset_before_jump(void) {
    tsb_reset_before_jump();
}
