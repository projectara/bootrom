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

#ifndef __COMMON_INCLUDE_GBBOOT_FAKE_SVC_H
#define __COMMON_INCLUDE_GBBOOT_FAKE_SVC_H

int svc_wait_for_peer_ready(void);

/*
 * @brief UniPro connections
 */
struct unipro_connection {
    uint8_t port_id0;
    uint8_t device_id0;
    uint16_t cport_id0;
    uint8_t port_id1;
    uint8_t device_id1;
    uint16_t cport_id1;
    uint8_t tc;
    uint8_t flags;
    uint8_t state;
};

struct fake_switch {
    unsigned char not_used;
};

#define SWITCH_UNIPORT_MAX          (14)
#define SWITCH_PORT_ID              SWITCH_UNIPORT_MAX
#define SWITCH_PORT_MAX             (SWITCH_UNIPORT_MAX + 1)

int switch_set_local_dev_id(struct fake_switch *sw,
                            uint8_t port_id,
                            uint8_t dev_id);

int switch_if_dev_id_set(struct fake_switch *sw,
                         uint8_t port_id,
                         uint8_t dev_id);

int switch_cport_disconnect(struct fake_switch *sw,
                            uint8_t port_id0,
                            uint8_t cport_id0,
                            uint8_t port_id1,
                            uint8_t cport_id1);

int switch_cport_connect(struct fake_switch *sw,
                         struct unipro_connection *c);

void switch_gear_change(uint32_t gear,
                        uint32_t termination,
                        uint32_t hsseries,
                        uint32_t num_of_lanes,
                        uint32_t powermode);

#endif /* __COMMON_INCLUDE_GBBOOT_FAKE_SVC_H */
