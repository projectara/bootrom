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
#include "bootrom.h"
#include "chipapi.h"
#include "debug.h"
#include "greybus.h"
#include "gbfirmware.h"

uint32_t gbfw_cportid = 0;

extern unsigned char build_manifest_manifest_mnfb[];
extern unsigned int build_manifest_manifest_mnfb_len;

static int greybus_send_message(uint32_t cport,
                                uint16_t id,
                                uint8_t type,
                                uint8_t status,
                                unsigned char *payload_data,
                                uint16_t payload_size) {
    /**
     * the payload_size are all pretty small for Greybus Control protocol
     * and the firmware downloading protocol on the boot ROM side.
     * So we can use the variable sized array here and not worrying
     * about running out of stack space
     */
    unsigned char msg[sizeof(gb_operation_header) + payload_size];

    gb_operation_header *msg_header = (gb_operation_header *)msg;
    unsigned char *payload = &msg[sizeof(gb_operation_header)];

    msg_header->size    = sizeof(msg);
    msg_header->id      = id;
    msg_header->type    = type;
    msg_header->status  = status;
    msg_header->padding = 0;

    if (payload_size != 0 && payload_data != NULL) {
        memcpy(payload, payload_data, payload_size);
    }
    return chip_unipro_send(cport, msg, sizeof(msg));
}

int greybus_send_request(uint32_t cport,
                         uint16_t id,
                         uint8_t type,
                         unsigned char *payload_data,
                         uint16_t payload_size) {
    return greybus_send_message(cport,
                                id,
                                type,
                                0,
                                payload_data,
                                payload_size);
}

int greybus_op_response(uint32_t cport,
                        gb_operation_header *op_header,
                        uint8_t status,
                        unsigned char *payload_data,
                        uint16_t payload_size) {
    return greybus_send_message(cport,
                                op_header->id,
                                op_header->type | GB_TYPE_RESPONSE,
                                status,
                                payload_data,
                                payload_size);
}

static int gbctrl_get_version(uint32_t cportid,
                            gb_operation_header *op_header) {
    unsigned char payload[2] = {GREYBUS_MAJOR_VERSION,
                                GREYBUS_MINOR_VERSION};

    return greybus_op_response(cportid,
                               op_header,
                               GB_OP_SUCCESS,
                               payload,
                               sizeof(payload));
}

static int gbctrl_probe_ap(uint32_t cportid,
                         gb_operation_header *op_header) {
    uint16_t payload[1] = {0};

    return greybus_op_response(cportid,
                               op_header,
                               GB_OP_SUCCESS,
                               (unsigned char*)payload,
                               sizeof(payload));
}

static int gbctrl_get_manifest_size(uint32_t cportid,
                                  gb_operation_header *op_header) {
    uint16_t payload[1] = {build_manifest_manifest_mnfb_len};

    return greybus_op_response(cportid,
                               op_header,
                               GB_OP_SUCCESS,
                               (unsigned char*)payload,
                               sizeof(payload));
}

static bool manifest_fetched = false;

bool manifest_fetched_by_ap(void) {
    return manifest_fetched;
}

static int gbctrl_get_manifest(uint32_t cportid,
                             gb_operation_header *op_header) {
    int rc;

    rc = greybus_op_response(cportid,
                             op_header,
                             GB_OP_SUCCESS,
                             build_manifest_manifest_mnfb,
                             build_manifest_manifest_mnfb_len);
    if (rc) {
        return rc;
    }

    manifest_fetched = true;
    return 0;
}

static int gbctrl_connected(uint32_t cportid,
                          gb_operation_header *op_header) {
    int rc;
    uint16_t *payload = (uint16_t *)(op_header + 1);

    if (op_header->size != sizeof(gb_operation_header) + sizeof(*payload)) {
        greybus_op_response(cportid,
                            op_header,
                            GB_OP_INVALID,
                            NULL,
                            0);
        return -1;
    }

    rc = greybus_cport_connect();
    if (rc != 0 || *payload != gbfw_cportid) {
        greybus_op_response(cportid,
                            op_header,
                            GB_OP_UNKNOWN_ERROR,
                            NULL,
                            0);
        return -1;
    }
    return greybus_op_response(cportid,
                               op_header,
                               GB_OP_SUCCESS,
                               NULL,
                               0);
}

static int gbctrl_disconnected(uint32_t cportid,
                             gb_operation_header *op_header) {
    int rc;
    uint16_t *payload = (uint16_t *)(op_header + 1);

    if (op_header->size != sizeof(gb_operation_header) + sizeof(*payload) ||
        *payload != gbfw_cportid) {
        greybus_op_response(cportid,
                            op_header,
                            GB_OP_INVALID,
                            NULL,
                            0);
        return -1;
    }

    rc = greybus_cport_disconnect();
    if (rc != 0) {
        greybus_op_response(cportid,
                            op_header,
                            GB_OP_UNKNOWN_ERROR,
                            NULL,
                            0);
        return -1;
    }
    return greybus_op_response(cportid,
                               op_header,
                               GB_OP_SUCCESS,
                               NULL,
                               0);
}

int control_cport_handler(uint32_t cportid,
                          void *data,
                          size_t len)
{
    int rc = 0;
    if (len < sizeof(gb_operation_header)) {
        dbgprint("control_cport_handler: RX data length error\r\n");
        return -1;
    }

    gb_operation_header *op_header = (gb_operation_header *)data;

    switch (op_header->type) {
    case GB_CTRL_OP_VERSION:
        rc = gbctrl_get_version(cportid, op_header);
        break;
    case GB_CTRL_OP_PROBE_AP:
        rc = gbctrl_probe_ap(cportid, op_header);
        break;
    case GB_CTRL_OP_GET_MANIFEST_SIZE:
        rc = gbctrl_get_manifest_size(cportid, op_header);
        break;
    case GB_CTRL_OP_GET_MANIFEST:
        rc = gbctrl_get_manifest(cportid, op_header);
        break;
    case GB_CTRL_OP_CONNECTED:
        rc = gbctrl_connected(cportid, op_header);
        break;
    case GB_CTRL_OP_DISCONNECTED:
        rc = gbctrl_disconnected(cportid, op_header);
        break;
    default:
        break;
    }

    return rc;
}
