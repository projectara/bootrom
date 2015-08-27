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
#include <errno.h>
#include "bootrom.h"
#include "chipapi.h"
#include "chipdef.h"
#include "debug.h"
#include "unipro.h"
#include "tsb_unipro.h"
#include "greybus.h"
#include "data_loading.h"
#include "gbfirmware.h"
#include "crypto.h"

/* Greybus FirmWare protocol version we support */
#define GB_FIRMWARE_VERSION_MAJOR   0x00
#define GB_FIRMWARE_VERSION_MINOR   0x01

static uint8_t responded_op = GB_FW_OP_INVALID;

int fw_cport_handler(uint32_t cportid, void *data, size_t len);

static int gbfw_get_version(uint32_t cportid, gb_operation_header *header) {
    uint8_t payload[2] = {GB_FIRMWARE_VERSION_MAJOR, GB_FIRMWARE_VERSION_MINOR};
    return greybus_op_response(cportid, header, GB_OP_SUCCESS, payload,
                               sizeof(payload));
}

static int gbfw_ap_ready(uint32_t cportid, gb_operation_header *header) {
    return greybus_op_response(cportid, header, GB_OP_SUCCESS, NULL, 0);
}

static struct gbfw_firmware_size_response firmware_size_response;

static int gbfw_firmware_size(uint8_t stage, uint32_t *size) {
    int rc;
    struct gbfw_firmware_size_request req = {stage};
    rc = greybus_send_request(gbfw_cportid, 1, GB_FW_OP_FIRMWARE_SIZE,
                              (uint8_t*)&req, sizeof(req));
    if (rc) {
        return rc;
    }

    rc = chip_unipro_receive(gbfw_cportid, fw_cport_handler);
    if (rc) {
        return rc;
    }

    *size = firmware_size_response.size;
    return 0;
}

static int gbfw_firmware_size_response(gb_operation_header *head, void *data,
                                       uint32_t len) {
    if(len < sizeof(struct gbfw_firmware_size_response)) {
        dbgprint("gbfw_firmware_size_response: incorrect response size\r\n");
        return GB_FW_ERR_INVALID;
    }
    memcpy(&firmware_size_response, data, sizeof(firmware_size_response));
    return 0;
}

static struct fw_get_firmware_buff {
    uint8_t *buffer;
    uint32_t size;
} fw_get_firmware_buff;

static int gbfw_get_firmware(uint32_t offset, uint32_t size, void *data,
                             void* prev, uint32_t prev_len) {
    int rc;
    struct gbfw_get_firmware_request req = {offset, size};
    rc = greybus_send_request(gbfw_cportid, 1, GB_FW_OP_GET_FIRMWARE,
                              (uint8_t*)&req, sizeof(req));
    if (rc) {
        return rc;
    }

    if (prev_len > 0) {
        hash_update((unsigned char*)prev, prev_len);
    }

    fw_get_firmware_buff.buffer = data;
    fw_get_firmware_buff.size   = size;

    rc = chip_unipro_receive(gbfw_cportid, fw_cport_handler);
    if (rc) {
        dbgprintx32("failed to receive firmware: -", -rc, "\r\n");
        return rc;
    }
    if (responded_op != (GB_FW_OP_GET_FIRMWARE | GB_TYPE_RESPONSE)) {
        dbgprint("Response wasn't a get-firmware response\r\n");
        return -ENODEV;
    }

    return 0;
}

static int gbfw_get_firmware_response(gb_operation_header *header, void *data,
                                      uint32_t len) {
    if (header->size - sizeof(gb_operation_header) != len ||
        len != fw_get_firmware_buff.size) {
        dbgprint("gbfw_get_firmware_response(): incorrect response size\r\n");
        return GB_FW_ERR_INVALID;
    }
    if (header->status) {
        dbgprint("gbfw_get_firmware_response(): error status in response\r\n");
        return -header->status;
    }
    memcpy(fw_get_firmware_buff.buffer, data, fw_get_firmware_buff.size);
    return 0;
}

static int gbfw_ready_to_boot(uint8_t status) {
    int rc;
    struct gbfw_ready_to_boot_request req = {status};
    rc = greybus_send_request(gbfw_cportid, 1, GB_FW_OP_READY_TO_BOOT,
                              (uint8_t*)&req, sizeof(req));
    if (rc) {
        return rc;
    }

    rc = chip_unipro_receive(gbfw_cportid, fw_cport_handler);
    if (rc) {
        return rc;
    }

    return 0;
}

static int gbfw_ready_to_boot_response(gb_operation_header *header, void *data,
                                       uint32_t len) {
    if (header->status) {
        dbgprint("gbfw_ready_to_boot_response(): error status in response\r\n");
        return -header->status;
    }
    return 0;
}

int fw_cport_handler(uint32_t cportid, void *data, size_t len) {
    int rc = 0;
    if (cportid != gbfw_cportid) {
        dbgprint("fw_cport_handler: incorrect CPort number");
        return GB_FW_ERR_INVALID;
    }
    if (len < sizeof(gb_operation_header)) {
        dbgprint("fw_cport_handler: RX data length error\r\n");
        return GB_FW_ERR_INVALID;
    }

    gb_operation_header *op_header = (gb_operation_header *)data;
    if(op_header->size < len) {
        dbgprint("fw_cport_handler: nonsense message received.\r\n");
        return GB_FW_ERR_INVALID;
    }
    /*
     * This works here because we're actually calling this handler
     * synchronously.  We set the constant and return to the recipient rather
     * than setting the constant in parallel with the recipient.
     */
    responded_op = op_header->type;
    data += sizeof(gb_operation_header);
    len -= sizeof(gb_operation_header);
    switch (op_header->type) {
    case GB_FW_OP_PROTOCOL_VERSION:
        rc = gbfw_get_version(cportid, op_header);
        break;
    case GB_FW_OP_FIRMWARE_SIZE | GB_TYPE_RESPONSE:
        rc = gbfw_firmware_size_response(op_header, data, len);
        break;
    case GB_FW_OP_GET_FIRMWARE | GB_TYPE_RESPONSE:
        rc = gbfw_get_firmware_response(op_header, data, len);
        break;
    case GB_FW_OP_READY_TO_BOOT | GB_TYPE_RESPONSE:
        rc = gbfw_ready_to_boot_response(op_header, data, len);
        break;
    case GB_FW_OP_AP_READY:
        rc = gbfw_ap_ready(cportid, op_header);
        break;
    default:
        responded_op = GB_FW_OP_INVALID;
        break;
    }

    if (rc) {
        responded_op = GB_FW_OP_INVALID;
    }

    return rc;
}

static int cport_connected = 0, offset = -1;
static uint32_t firmware_size = 0;
int greybus_cport_connect(void) {
    int rc;

    if (cport_connected == 1) {
        /* Don't know what to do if it is already connected */
        return GB_FW_ERR_INVALID;
    }
    rc = chip_unipro_recv_cport(&gbfw_cportid);
    if (rc) {
        return rc;
    }

    offset = 0;
    cport_connected = 1;
    return 0;
}

int greybus_cport_disconnect(void) {
    if (cport_connected == 0) {
        return -EINVAL;
    }
    return 0;
}

static int data_load_greybus_init(void) {
    int rc;

    rc = chip_unipro_init_cport(CONTROL_CPORT);
    if (rc) {
        return rc;
    }

    /* poll until data cport connected */
    while (cport_connected == 0) {
        rc = chip_unipro_receive(CONTROL_CPORT, control_cport_handler);
        if (rc == GB_FW_ERR_INVALID) {
            dbgprint("Greybus init failed\r\n");
        }
        if (rc) {
            goto protocol_error;
        }
    }

    /* Spin until the AP asks for our protocol version. */
    while(responded_op != GB_FW_OP_AP_READY) {
        rc = chip_unipro_receive(gbfw_cportid, fw_cport_handler);
        if (rc) {
            dbgprint("Greybus Firmware CPort handler failed\r\n");
            goto protocol_error;
        }
    }

    dbgprint("Beginning Greybus Firmware download.\r\n");

    /* Fetch the firmware size. */
    rc = gbfw_firmware_size(NEXT_BOOT_STAGE, &firmware_size);
    if (rc) {
        goto protocol_error;
    }
    if (firmware_size > WORKRAM_SIZE) {
        rc = -EINVAL;
        goto protocol_error;
    }

    return 0;

protocol_error:
    cport_connected = 0;
    return rc;
}

static int data_load_greybus_load(void *dest, uint32_t length, bool hash) {
    int rc;
    uint32_t blk_len, prev_len = 0;
    void *prev = NULL;
    if (cport_connected != 1 || offset + length > firmware_size) {
        return GB_FW_ERR_INVALID;
    }

    while (length) {
        blk_len = (length > GB_MAX_PAYLOAD_SIZE) ? GB_MAX_PAYLOAD_SIZE : length;
        rc = gbfw_get_firmware(offset, blk_len, dest, prev, prev_len);
        if (rc) {
            return rc;
        }

        if (hash) {
            prev = dest;
            prev_len = blk_len;
        }

        dest   += blk_len;
        offset += blk_len;
        length -= blk_len;
    }

    if (hash) {
        hash_update((unsigned char*)prev, prev_len);
    }

    return 0;
}

static int data_load_greybus_finish(bool valid, bool is_secure_image) {
    int rc;
    uint8_t status = GB_FW_BOOT_STATUS_INVALID;
    if (valid && is_secure_image) {
        status = GB_FW_BOOT_STATUS_SECURE;
    }
    else if (valid) {
        status = GB_FW_BOOT_STATUS_INSECURE;
    }

    rc = gbfw_ready_to_boot(status);

    dbgprint("Finished Greybus Firmware download.\r\n");

    firmware_size = offset = -1;
    cport_connected = 0;
    return rc;
}

data_load_ops greybus_ops = {
    .init = data_load_greybus_init,
    .read = NULL,
    .load = data_load_greybus_load,
    .finish = data_load_greybus_finish
};
