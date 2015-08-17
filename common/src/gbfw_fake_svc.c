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

#include <stdint.h>
#include "chipapi.h"
#include "common.h"
#include "unipro.h"
#include "debug.h"
#include "data_loading.h"
#include "tftf.h"
#include "ffff.h"
#include "tsb_unipro.h"
#include "gbfw_fake_svc.h"

int svc_wait_for_peer_ready(void) {
    uint32_t val;
    uint32_t result;
    int rc;

    while (1) {
        rc = read_mailbox(&val, &result);
        if (rc || result) {
            dbgprint("Error when waiting for ready\r\n");
            return -1;
        }
        if (val != TSB_MAIL_RESET) {
            dbgprintx32("Peer ready: ", val, "\r\n");
            ack_mailbox();
            break;
        }
    }

    return 0;
}

#define SWITCH_UNIPORT_MAX          (14)
#define SWITCH_PORT_ID              SWITCH_UNIPORT_MAX
#define SWITCH_PORT_MAX             (SWITCH_UNIPORT_MAX + 1)

/* Default CPort configuration values. */
#define CPORT_DEFAULT_TOKENVALUE           32
#define CPORT_DEFAULT_T_PROTOCOLID         0
#define CPORT_DEFAULT_TSB_MAXSEGMENTCONFIG 0x118

static int switch_dme_set(struct fake_switch *sw,
                          uint8_t portid,
                          uint16_t attrid,
                          uint16_t select_index,
                          uint32_t attr_value) {
    uint32_t res;
    chip_unipro_attr_write(attrid,
                           attr_value,
                           select_index,
                           0,
                           &res);
    return -res;
}

static int switch_dme_get(struct fake_switch *sw,
                          uint8_t portid,
                          uint16_t attrid,
                          uint16_t select_index,
                          uint32_t *attr_value) {
    uint32_t res;
    chip_unipro_attr_read(attrid,
                          attr_value,
                          select_index,
                          0,
                          &res);
    return -res;
}

static int switch_dme_peer_set(struct fake_switch *sw,
                               uint8_t portid,
                               uint16_t attrid,
                               uint16_t select_index,
                               uint32_t attr_value) {
    uint32_t res;
    chip_unipro_attr_write(attrid,
                           attr_value,
                           select_index,
                           1,
                           &res);
    return -res;
}

static int switch_dme_peer_get(struct fake_switch *sw,
                               uint8_t portid,
                               uint16_t attrid,
                               uint16_t select_index,
                               uint32_t *attr_value) {
    uint32_t res;
    chip_unipro_attr_read(attrid,
                          attr_value,
                          select_index,
                          0,
                          &res);
    return -res;
}

static int switch_set_port_l4attr(struct fake_switch *sw,
                                  uint8_t portid,
                                  uint16_t attrid,
                                  uint16_t selector,
                                  uint32_t val) {
    int rc;

    if (portid == SWITCH_PORT_ID) {
        rc = switch_dme_set(sw, portid, attrid, selector, val);
    } else {
        rc = switch_dme_peer_set(sw, portid, attrid, selector, val);
    }

    return rc;
}

static int switch_get_port_l4attr(struct fake_switch *sw,
                                  uint8_t portid,
                                  uint16_t attrid,
                                  uint16_t selector,
                                  uint32_t *val) {
    int rc;

    if (portid == SWITCH_PORT_ID) {
        rc = switch_dme_get(sw, portid, attrid, selector, val);
    } else {
        rc = switch_dme_peer_get(sw, portid, attrid, selector, val);
    }

    return rc;
}

static int switch_set_pair_attr(struct fake_switch *sw,
                                struct unipro_connection *c,
                                uint16_t attrid,
                                uint32_t val0,
                                uint32_t val1) {
    int rc;
    rc = switch_set_port_l4attr(sw,
            c->port_id0,
            attrid,
            c->cport_id0,
            val0);
    if (rc) {
        return rc;
    }

    rc = switch_set_port_l4attr(sw,
            c->port_id1,
            attrid,
            c->cport_id1,
            val1);
    if (rc) {
        return rc;
    }

    return 0;
}

int switch_cport_connect(struct fake_switch *sw,
                         struct unipro_connection *c) {
    int e2efc_enabled = (!!(c->flags & CPORT_FLAGS_E2EFC) == 1);
    int csd_enabled = (!!(c->flags & CPORT_FLAGS_CSD_N) == 0);
    int rc = 0;

    /* Disable any existing connection(s). */
    rc = switch_set_pair_attr(sw, c, T_CONNECTIONSTATE, 0, 0);
    if (rc) {
        return rc;
    }

    /*
     * Point each device at the other.
     */
    rc = switch_set_pair_attr(sw,
                              c,
                              T_PEERDEVICEID,
                              c->device_id1,
                              c->device_id0);
    if (rc) {
        return rc;
    }

    /*
     * Point each CPort at the other.
     */
    rc = switch_set_pair_attr(sw, c, T_PEERCPORTID, c->cport_id1, c->cport_id0);
    if (rc) {
        return rc;
    }

    /*
     * Match up traffic classes.
     */
    rc = switch_set_pair_attr(sw, c, T_TRAFFICCLASS, c->tc, c->tc);
    if (rc) {
        return rc;
    }

    /*
     * Make sure the protocol IDs are equal. (We don't use them otherwise.)
     */
    rc = switch_set_pair_attr(sw,
                              c,
                              T_PROTOCOLID,
                              CPORT_DEFAULT_T_PROTOCOLID,
                              CPORT_DEFAULT_T_PROTOCOLID);
    if (rc) {
        return rc;
    }

    /*
     * Set default TxTokenValue and RxTokenValue values.
     *
     * IMPORTANT: TX and RX token values must be equal if E2EFC is
     * enabled, so don't change them to different values unless you
     * also patch up the E2EFC case, below.
     */
    rc = switch_set_pair_attr(sw,
                              c,
                              T_TXTOKENVALUE,
                              CPORT_DEFAULT_TOKENVALUE,
                              CPORT_DEFAULT_TOKENVALUE);
    if (rc) {
        return rc;
    }

    rc = switch_set_pair_attr(sw,
                              c,
                              T_RXTOKENVALUE,
                              CPORT_DEFAULT_TOKENVALUE,
                              CPORT_DEFAULT_TOKENVALUE);
    if (rc) {
        return rc;
    }


    /*
     * Set CPort flags.
     *
     * (E2EFC needs to be the same on both sides, which is handled by
     * having a single flags value for now.)
     */
    rc = switch_set_pair_attr(sw, c, T_CPORTFLAGS, c->flags, c->flags);
    if (rc) {
        return rc;
    }

    /*
     * If E2EFC is enabled, or E2EFC is disabled and CSD is enabled,
     * then each CPort's T_PeerBufferSpace must equal the peer CPort's
     * T_LocalBufferSpace.
     */
    if (e2efc_enabled || (!e2efc_enabled && csd_enabled)) {
        uint32_t cport0_local = 0;
        uint32_t cport1_local = 0;
        rc = switch_get_port_l4attr(sw,
                c->port_id0,
                T_LOCALBUFFERSPACE,
                c->cport_id0,
                &cport0_local);
        if (rc) {
            return rc;
        }

        rc = switch_get_port_l4attr(sw,
                c->port_id1,
                T_LOCALBUFFERSPACE,
                c->cport_id1,
                &cport1_local);
        if (rc) {
            return rc;
        }

        rc = switch_set_pair_attr(sw,
                                  c,
                                  T_LOCALBUFFERSPACE,
                                  cport0_local,
                                  cport1_local);
        if (rc) {
            return rc;
        }
    }

    /*
     * Ensure the CPorts aren't in test mode.
     */
    rc = switch_set_pair_attr(sw,
                              c,
                              T_CPORTMODE,
                              CPORT_MODE_APPLICATION,
                              CPORT_MODE_APPLICATION);
    if (rc) {
        return rc;
    }

    /*
     * Clear out the credits to send on each side.
     */
    rc = switch_set_pair_attr(sw, c, T_CREDITSTOSEND, 0, 0);
    if (rc) {
        return rc;
    }

    /*
     * XXX Toshiba-specific TSB_MaxSegmentConfig (move to bridge ASIC code.)
     */
    rc = switch_set_pair_attr(sw,
                              c,
                              TSB_MAXSEGMENTCONFIG,
                              CPORT_DEFAULT_TSB_MAXSEGMENTCONFIG,
                              CPORT_DEFAULT_TSB_MAXSEGMENTCONFIG);
    if (rc) {
        return rc;
    }
    /*
     * Establish the connections!
     */
    rc = switch_set_pair_attr(sw, c, T_CONNECTIONSTATE, 1, 1);
    if (rc) {
        return rc;
    }

    return rc;
}

int switch_cport_disconnect(struct fake_switch *sw,
                            uint8_t port_id0,
                            uint8_t cport_id0,
                            uint8_t port_id1,
                            uint8_t cport_id1) {
    int rc0, rc1;
    rc0 = switch_dme_peer_set(sw, port_id0, T_CONNECTIONSTATE,
                             cport_id0, 0x0);
    rc1 = switch_dme_peer_set(sw, port_id1, T_CONNECTIONSTATE,
                              cport_id1, 0x0);
    return rc0 || rc1;
}

/**
 * @brief Assign a device id to a given port id
 */
int switch_if_dev_id_set(struct fake_switch *sw,
                         uint8_t port_id,
                         uint8_t dev_id) {
    int rc;

    if (port_id >= SWITCH_UNIPORT_MAX) {
        return -1;
    }

    rc = switch_dme_peer_set(sw, port_id, N_DEVICEID,
                             UNIPRO_SELINDEX_NULL, dev_id);
    if (rc) {
        return rc;
    }

    rc = switch_dme_peer_set(sw, port_id, N_DEVICEID_VALID,
                             UNIPRO_SELINDEX_NULL, 1);
    if (rc) {
        /* do what on failure? */
        return rc;
    }

    return 0;
}

int switch_set_local_dev_id(struct fake_switch *sw,
                            uint8_t port_id,
                            uint8_t dev_id) {
    int rc;

    if (port_id >= SWITCH_UNIPORT_MAX) {
        return -1;
    }

    rc = switch_dme_set(sw, port_id, N_DEVICEID,
                        UNIPRO_SELINDEX_NULL, dev_id);
    if (rc) {
        return rc;
    }

    rc = switch_dme_set(sw, port_id, N_DEVICEID_VALID,
                        UNIPRO_SELINDEX_NULL, 1);
    if (rc) {
        /* do what on failure? */
        return rc;
    }

    return 0;
}
