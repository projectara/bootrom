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
#include "appcfg.h"
#include "chipcfg.h"
#include "bootrom.h"
#include "chipapi.h"
#include "debug.h"
#include "greybus.h"
#include "gbboot.h"
#include "ara_mailbox.h"


extern unsigned char manifest_mnfb[];
extern unsigned int manifest_mnfb_len;

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
    uint16_t payload[1] = {manifest_mnfb_len};

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
                             manifest_mnfb,
                             manifest_mnfb_len);
    if (rc) {
        return rc;
    }

    manifest_fetched = true;
    return 0;
}

static int gbctrl_connected(uint32_t cportid,
                          gb_operation_header *op_header) {
    uint16_t *payload = (uint16_t *)(op_header + 1);

    if (op_header->size != sizeof(gb_operation_header) + sizeof(*payload)) {
        greybus_op_response(cportid,
                            op_header,
                            GB_OP_INVALID,
                            NULL,
                            0);
        return -1;
    }

    chip_unipro_init_cport(*payload);

    return greybus_op_response(cportid,
                               op_header,
                               GB_OP_SUCCESS,
                               NULL,
                               0);
}

static int gbctrl_disconnected(uint32_t cportid,
                             gb_operation_header *op_header) {
    uint16_t *payload = (uint16_t *)(op_header + 1);

    if (op_header->size != sizeof(gb_operation_header) + sizeof(*payload)) {
        greybus_op_response(cportid,
                            op_header,
                            GB_OP_INVALID,
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

static greybus_op_handler *handler_table[CPORT_MAX];

int common_cport_handler(uint32_t cportid,
                         void *data,
                         size_t len)
{
    int rc = 0;
    int i;

    /* If the message is longer than the buffer, it will have been rejected
     * by the Rx function.
     * If the message is shorter than the buffer but longer than the specific
     * message type, the remainder of the buffer is silently and benignly
     * ignored.
     * */
    if (len < sizeof(gb_operation_header)) {
        dbgprintx32("cport ", cportid, " RX data length error\r\n");
        return -1;
    }

    gb_operation_header *op_header = (gb_operation_header *)data;

    greybus_op_handler_func handler = NULL;
    i = 0;
    while (handler_table[cportid] != NULL &&
           handler_table[cportid][i].type != HANDLER_TABLE_END) {
        if (handler_table[cportid][i].type == op_header->type) {
            handler = handler_table[cportid][i].handler;
            break;
        }
        i++;
    }

    if (handler == NULL) {
        dbgprint("Failed to find greybus operation handler\n");
        /* treat unknown operations as error */
        return -1;
    }

    rc = handler(cportid, op_header);
    return rc;
}

static greybus_op_handler control_cport_handlers[] = {
    {GB_CTRL_OP_VERSION, gbctrl_get_version},
    {GB_CTRL_OP_PROBE_AP, gbctrl_probe_ap},
    {GB_CTRL_OP_GET_MANIFEST_SIZE, gbctrl_get_manifest_size},
    {GB_CTRL_OP_GET_MANIFEST, gbctrl_get_manifest},
    {GB_CTRL_OP_CONNECTED, gbctrl_connected},
    {GB_CTRL_OP_DISCONNECTED, gbctrl_disconnected},
    {HANDLER_TABLE_END, NULL}
};

void greybus_register_handlers(uint32_t cportid,
                               greybus_op_handler *handlers) {
    handler_table[cportid] = handlers;
}

int greybus_init(void) {
    int i;
    int rc;

    for (i = 0; i < CPORT_MAX; i++) {
        handler_table[i] = NULL;
    }

    rc = chip_unipro_init_cport(CONTROL_CPORT);
    greybus_register_handlers(CONTROL_CPORT, control_cport_handlers);
    return rc;
}

int greybus_loop(void) {
    int rc;
    uint32_t cportid;
    uint32_t mbox;

    while(1) {
        if (is_mailbox_irq_pending()) {
            chip_unipro_recv_cport(&mbox);
        }
        for (cportid = 0; cportid < CPORT_MAX; cportid++) {
            if (handler_table[cportid]) {
                rc = unipro_receive(cportid, common_cport_handler);
                if (rc < 0) {
                    return -1;
                }
                if (rc > 0) {
                    return 0;
                }
            }
        }
    }
    return 0;
}
