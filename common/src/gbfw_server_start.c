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
#include "greybus.h"
#include "debug.h"
#include "data_loading.h"
#include "ffff.h"
#include "gbfw_fake_svc.h"
#include "utils.h"
#include "gbfirmware.h"

extern data_load_ops spi_ops;
uint32_t br_errno;

static void server_loop(void);

/* the values below do not really matter in our environment */
#define LOCAL_DEV_ID 0xA
#define PEER_DEV_ID 0xB
#define GBFW_CPORT 1
#define CLIENT_DATA_CPORT 1
#define PEER_PORT_ID 1
/**
 * @brief Bootloader "C" entry point
 *
 * @param none
 *
 * @returns Nothing. Will launch/restart image if successful, halt if not.
 */
void bootrom_main(void) {
    chip_init();

    dbginit();

    dbgprint("GBFW Server\n");

    chip_wait_for_link_up();
    while(1) {
        server_loop();
    }
}

struct unipro_connection conn[] = {
    {
        .port_id0 = SWITCH_PORT_ID,
        .device_id0 = LOCAL_DEV_ID,
        .cport_id0  = CONTROL_CPORT,
        .port_id1 = PEER_PORT_ID,
        .device_id1 = PEER_DEV_ID,
        .cport_id1  = CONTROL_CPORT,
        .flags      = 7,  /* from the boot sequence doc */
    },
    {
        .port_id0 = SWITCH_PORT_ID,
        .device_id0 = LOCAL_DEV_ID,
        .cport_id0  = GBFW_CPORT,
        .port_id1 = PEER_PORT_ID,
        .device_id1 = PEER_DEV_ID,
        .cport_id1  = CLIENT_DATA_CPORT,
        .flags      = 7,  /* from the boot sequence doc */
    },
};

static int server_control_cport_handler(uint32_t cportid,
                                        void *data,
                                        size_t len) {
    dbgprint("server ctrl cport Rx:");
    unsigned char *p = (unsigned char *)data;
    int i;
    for(i = 0; i < len; i++) {
        if ((i & 0xF) == 0) dbgprint("\n    ");
        dbgprinthex8(p[i]);dbgprint(" ");
    }
    dbgprint("\n");

    return 0;
}

int poke_mailbox(uint32_t val, int peer, uint32_t *result_code) {
    int rc;
    uint32_t result = 0;

    rc = chip_unipro_attr_write(TSB_MAILBOX, val, 0, peer, &result);
    if (rc) {
        return rc;
    }
    return 0;
}

int wait_for_mailbox_ack(int peer, uint32_t *result_code) {
    int rc;
    uint32_t result = 0, read_result = 0, val = 0;
    do {
        rc = chip_unipro_attr_read(TSB_MAILBOX, &val, 0, peer, &read_result);
    } while (!rc && val != TSB_MAIL_RESET);
    if (rc) {
        return rc;
    }

    if (result_code) {
        *result_code = result;
    }
    return 0;
}

int create_connection(struct unipro_connection *c) {
    uint32_t result;
    switch_cport_connect(NULL, c);

    /**
     * This part (poking local mailbox) is not part of the greybus spec.
     * It is here so we can re-use the existing unipro code
     */
    poke_mailbox(c->cport_id0 + 1, 0, &result);
    chip_unipro_init_cport(c->cport_id0);
    wait_for_mailbox_ack(0, &result);

    write_mailbox(c->cport_id1 + 1, &result);
    if (result) {
        dbgprintx32("Couldn't poke mailbox for connecting cport ",
                    c->cport_id1 + 1,
                    "\n");
            return -1;
    }
    return 0;
}

static int gb_control(void) {
    unsigned char ver[] = {0, 1};
    greybus_send_request(CONTROL_CPORT,
                         1,
                         GB_CTRL_OP_VERSION,
                         ver,
                         2);
    chip_unipro_receive(CONTROL_CPORT, server_control_cport_handler);
    greybus_send_request(CONTROL_CPORT,
                         1,
                         GB_CTRL_OP_PROBE_AP,
                         ver,
                         2);
    chip_unipro_receive(CONTROL_CPORT, server_control_cport_handler);
    greybus_send_request(CONTROL_CPORT,
                         1,
                         GB_CTRL_OP_GET_MANIFEST_SIZE,
                         NULL,
                         0);
    chip_unipro_receive(CONTROL_CPORT, server_control_cport_handler);
    greybus_send_request(CONTROL_CPORT,
                         1,
                         GB_CTRL_OP_GET_MANIFEST,
                         NULL,
                         0);
    chip_unipro_receive(CONTROL_CPORT, server_control_cport_handler);
    struct unipro_connection *c = &conn[1];
    uint16_t to_connect = c->cport_id1;
    greybus_send_request(CONTROL_CPORT,
                         1,
                         GB_CTRL_OP_CONNECTED,
                         (unsigned char *)&to_connect,
                         sizeof(to_connect));

    create_connection(c);
    chip_unipro_receive(CONTROL_CPORT, server_control_cport_handler);
    return 0;
}

static bool image_download_finished = false;
static int gbfw_get_firmware_size(uint32_t cportid,
                                  gb_operation_header *op_header) {
    int rc;
    uint8_t *payload = (uint8_t *)op_header + sizeof(*op_header);
    uint32_t size;
    spi_ops.init();
    rc = locate_ffff_element_on_storage(&spi_ops, *payload - 1, &size);

    dbgprintx32("image size: ", size, "\n");
    return greybus_op_response(cportid,
                               op_header,
                               (rc == 0) ? GB_OP_SUCCESS : GB_OP_UNKNOWN_ERROR,
                               (unsigned char*)&size,
                               sizeof(size));
}

static int gbfw_get_firmware(uint32_t cportid,
                             gb_operation_header *op_header) {
    int rc;
    uint8_t *payload = (uint8_t *)op_header + sizeof(*op_header);
    struct __attribute__ ((packed)) get_fw_req {
        uint32_t offset;
        uint32_t size;
    } *req = (struct get_fw_req *)payload;
    uint8_t data[req->size];

    rc = spi_ops.load(data, req->size, false);

    return greybus_op_response(cportid,
                               op_header,
                               (rc == 0) ? GB_OP_SUCCESS : GB_OP_UNKNOWN_ERROR,
                               data,
                               req->size);
}

static int gbfw_ready_to_boot(uint32_t cportid,
                              gb_operation_header *op_header) {
    uint8_t *payload = (uint8_t *)op_header + sizeof(*op_header);
    dbgprintx32("ready-to-boot, status: ", *payload, "\n");

    image_download_finished = true;
    return greybus_op_response(cportid,
                               op_header,
                               (*payload != 0) ? GB_OP_SUCCESS : GB_OP_UNKNOWN_ERROR,
                               NULL,
                               0);
}

static int gbfw_cport_handler(uint32_t cportid,
                              void *data,
                              size_t len) {
    dbgprint("GBFW cport Rx:");
    unsigned char *p = (unsigned char *)data;
    int i;
    for(i = 0; i < len; i++) {
        if ((i & 0xF) == 0) dbgprint("\n    ");
        dbgprinthex8(p[i]);dbgprint(" ");
    }
    dbgprint("\n");

    int rc = 0;
    if (len < sizeof(gb_operation_header)) {
        dbgprint("control_cport_handler: RX data length err\n");
        return -1;
    }

    gb_operation_header *op_header = (gb_operation_header *)data;

    switch (op_header->type) {
    case GB_FW_OP_FIRMWARE_SIZE:
        rc = gbfw_get_firmware_size(cportid, op_header);
        break;
    case GB_FW_OP_GET_FIRMWARE:
        rc = gbfw_get_firmware(cportid, op_header);
        break;
    case GB_FW_OP_READY_TO_BOOT:
        rc = gbfw_ready_to_boot(cportid, op_header);
        break;
    default:
        break;
    }

    return rc;
}

static int gbfw_process(void) {
    unsigned char ver[] = {0, 1};
    greybus_send_request(GBFW_CPORT,
                         1,
                         GB_FW_OP_PROTOCOL_VERSION,
                         ver,
                         2);
    chip_unipro_receive(GBFW_CPORT, gbfw_cport_handler);
    image_download_finished = false;
    while (!image_download_finished) {
        chip_unipro_receive(GBFW_CPORT, gbfw_cport_handler);
    }
    return 0;
}

static void server_loop(void) {
    int rc;

    chip_unipro_init();

    switch_set_local_dev_id(NULL, SWITCH_PORT_ID, LOCAL_DEV_ID);

    dbgprint("Wait for peer...\n");
    chip_reset_before_ready();

    rc = svc_wait_for_peer_ready();
    if (rc) {
        return;
    }

    switch_if_dev_id_set(NULL, PEER_PORT_ID, PEER_DEV_ID);

    create_connection(&conn[0]);
    dbgprint("Control port connected\n");

    gb_control();
    gbfw_process();
}
