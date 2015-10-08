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
#include "error.h"
#include "greybus.h"
#include "debug.h"
#include "data_loading.h"
#include "ffff.h"
#include "gbboot_fake_svc.h"
#include "utils.h"
#include "gbboot.h"
#include "chipdef.h"

extern data_load_ops spi_ops;

static void server_loop(void);

/* the values below do not really matter in our environment */
#define LOCAL_DEV_ID 0xA
#define PEER_DEV_ID 0xB
#define gbboot_CPORT 1
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

    init_last_error();

    dbgprint("gbboot Server\n");

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
        .flags      = 6,  /* no E2EFC */
    },
    {
        .port_id0 = SWITCH_PORT_ID,
        .device_id0 = LOCAL_DEV_ID,
        .cport_id0  = gbboot_CPORT,
        .port_id1 = PEER_PORT_ID,
        .device_id1 = PEER_DEV_ID,
        .cport_id1  = CLIENT_DATA_CPORT,
        .flags      = 6,  /* no E2EFC */
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

int poke_mailbox(uint32_t val, int peer) {
    int rc;

    rc = chip_unipro_attr_write(ARA_MAILBOX, val, 0, peer);
    if (rc) {
        return rc;
    }
    return 0;
}

int wait_for_mailbox_ack(uint32_t wval, int peer) {
    int rc;
    uint32_t val = 0;
    do {
        rc = chip_unipro_attr_read(ARA_MBOX_ACK_ATTR, &val, 0, peer);
    } while (!rc && val != wval);
    if (rc) {
        return rc;
    }

    val = 0;
    chip_unipro_attr_write(ARA_MBOX_ACK_ATTR, val, 0, peer);
    return 0;
}

int create_connection(struct unipro_connection *c) {
    switch_cport_connect(NULL, c);

    /**
     * This part (poking local mailbox) is not part of the greybus spec.
     * It is here so we can re-use the existing unipro code
     */
    poke_mailbox(c->cport_id0 + 1, 0);
    chip_unipro_init_cport(c->cport_id0);
    wait_for_mailbox_ack(c->cport_id0 + 1, 0);

    write_mailbox(c->cport_id1 + 1);
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
static int stage_to_load;
static int gbboot_get_firmware_size(uint32_t cportid,
                                  gb_operation_header *op_header) {
    int rc;
    uint8_t *payload = (uint8_t *)op_header + sizeof(*op_header);
    uint32_t size;
    spi_ops.init();

    stage_to_load = *payload - 1;
    rc = locate_ffff_element_on_storage(&spi_ops, stage_to_load, &size);

    dbgprintx32("image size: ", size, "\n");

#ifdef _GEAR_CHANGE_TEST
    switch_gear_change(GEAR_HS_G2,
                       TERMINATION_ON,
                       HS_MODE_A,
                       1,
                       POWERMODE_FAST);
#endif

    return greybus_op_response(cportid,
                               op_header,
                               (rc == 0) ? GB_OP_SUCCESS : GB_OP_UNKNOWN_ERROR,
                               (unsigned char*)&size,
                               sizeof(size));
}

static int gbboot_get_firmware(uint32_t cportid,
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

static int gbboot_ready_to_boot(uint32_t cportid,
                              gb_operation_header *op_header) {
    uint8_t *payload = (uint8_t *)op_header + sizeof(*op_header);
    dbgprintx32("ready-to-boot, status: ", *payload, "\n");

    image_download_finished = true;
#ifdef _GEAR_CHANGE_TEST
    switch_gear_change(GEAR_HS_G3,
                       TERMINATION_ON,
                       HS_MODE_A,
                       2,
                       POWERMODE_FAST);
#endif
    return greybus_op_response(cportid,
                               op_header,
                               (*payload != 0) ? GB_OP_SUCCESS : GB_OP_UNKNOWN_ERROR,
                               NULL,
                               0);
}

static int gbboot_cport_handler(uint32_t cportid,
                              void *data,
                              size_t len) {
    dbgprint("gbboot cport Rx:");
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
    case GB_BOOT_OP_FIRMWARE_SIZE:
        rc = gbboot_get_firmware_size(cportid, op_header);
        break;
    case GB_BOOT_OP_GET_FIRMWARE:
        rc = gbboot_get_firmware(cportid, op_header);
#ifdef _GEAR_CHANGE_TEST
    switch_gear_change(GEAR_HS_G2,
                       TERMINATION_ON,
                       HS_MODE_A,
                       2,
                       POWERMODE_FAST);
#endif
        break;
    case GB_BOOT_OP_READY_TO_BOOT:
        rc = gbboot_ready_to_boot(cportid, op_header);
        break;
    default:
        break;
    }

    return rc;
}

static int gbboot_process(void) {
    unsigned char ver[] = {0, 1};
    greybus_send_request(gbboot_CPORT,
                         1,
                         GB_BOOT_OP_PROTOCOL_VERSION,
                         ver,
                         2);
    chip_unipro_receive(gbboot_CPORT, gbboot_cport_handler);
    greybus_send_request(gbboot_CPORT,
                         1,
                         GB_BOOT_OP_AP_READY,
                         NULL,
                         0);
    chip_unipro_receive(gbboot_CPORT, gbboot_cport_handler);
    image_download_finished = false;
    while (!image_download_finished) {
        chip_unipro_receive(gbboot_CPORT, gbboot_cport_handler);
    }
    return 0;
}

int chip_enter_hibern8_server(void);

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

#ifdef _GEAR_CHANGE_TEST
    switch_gear_change(GEAR_HS_G1,
                       TERMINATION_ON,
                       HS_MODE_A,
                       2,
                       POWERMODE_FAST);
#endif

    switch_if_dev_id_set(NULL, PEER_PORT_ID, PEER_DEV_ID);

    create_connection(&conn[0]);
    dbgprint("Control port connected\n");

    gb_control();
    gbboot_process();
#ifdef _GBBOOT_SERVER_STANDBY
    if (stage_to_load == FFFF_ELEMENT_STAGE_2_FW)
        chip_enter_hibern8_server();
#endif
}

