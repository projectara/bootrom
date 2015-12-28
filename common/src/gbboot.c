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
#include "appcfg.h"
#include "bootrom.h"
#include "chipapi.h"
#include "chipdef.h"
#include "debug.h"
#include "error.h"
#include "unipro.h"
#include "tsb_unipro.h"
#include "greybus.h"
#include "data_loading.h"
#include "gbboot.h"
#include "crypto.h"

#if (GB_MAX_PAYLOAD_SIZE > CPORT_RX_BUF_SIZE)
    #error "Greybus maximal payload must be smaller than CPort RX buffer"
#endif

static uint32_t gbboot_cportid = GBBOOT_CPORT;

/* We are receiving a firmware package in TFTF format, and not a raw firmware
 * binary
 */
/* Greybus FirmWare protocol version we support */
#define GB_FIRMWARE_VERSION_MAJOR   0x00
#define GB_FIRMWARE_VERSION_MINOR   0x01

#define CPORT_POLLING_TIMEOUT       512

static uint8_t responded_op = GB_BOOT_OP_INVALID;

int fw_cport_handler(uint32_t cportid, void *data, size_t len);

static int gbboot_get_version(uint32_t cportid, gb_operation_header *header) {
    uint8_t payload[2] = {GB_FIRMWARE_VERSION_MAJOR, GB_FIRMWARE_VERSION_MINOR};
    return greybus_op_response(cportid, header, GB_OP_SUCCESS, payload,
                               sizeof(payload));
}

static int gbboot_get_vid_pid(uint32_t cportid, gb_operation_header *header) {
    struct gbboot_firmware_get_vid_pid response = {ara_vid, ara_pid};
    return greybus_op_response(cportid, header, GB_OP_SUCCESS, (uint8_t*)&response,
                               sizeof(response));
}

static int gbboot_ap_ready(uint32_t cportid, gb_operation_header *header) {
    int rc;
    rc = greybus_op_response(cportid, header, GB_OP_SUCCESS, NULL, 0);
    if (rc) {
        return rc;
    }
    /* return >0 to break out from greybus loop */
    return 1;
}

static struct gbboot_firmware_size_response firmware_size_response;

static int gbboot_firmware_size(uint8_t stage, uint32_t *size) {
    int rc;
    struct gbboot_firmware_size_request req = {stage};
    rc = greybus_send_request(gbboot_cportid, 1, GB_BOOT_OP_FIRMWARE_SIZE,
                              (uint8_t*)&req, sizeof(req));
    if (rc) {
        return rc;
    }

    /* following loop breaks out after getting the firmware_size_response */
    rc = greybus_loop();
    if (rc) {
        return rc;
    }

    *size = firmware_size_response.size;
    return 0;
}

static int gbboot_firmware_size_response(gb_operation_header *head, void *data,
                                       uint32_t len) {
    if (head->status) {
        dbgprint("gbboot_firmware_size_response(): err status\n");
        return -head->status;
    }
    if(len < sizeof(struct gbboot_firmware_size_response)) {
        dbgprint("gbboot_firmware_size_response: bad resp. size\n");
        return GB_BOOT_ERR_INVALID;
    }
    memcpy(&firmware_size_response, data, sizeof(firmware_size_response));
    /* return >0 to break out from greybus loop */
    return 1;
}

static struct fw_get_firmware_buff {
    uint8_t *buffer;
    uint32_t size;
} fw_get_firmware_buff;

static int gbboot_get_firmware(uint32_t offset, uint32_t size, void *data,
                             void* prev, uint32_t prev_len) {
    int rc;
    struct gbboot_get_firmware_request req = {offset, size};
    rc = greybus_send_request(gbboot_cportid, 1, GB_BOOT_OP_GET_FIRMWARE,
                              (uint8_t*)&req, sizeof(req));
    if (rc) {
        return rc;
    }

    /* prev_len is set only when hash is required */
    if (prev_len > 0) {
        hash_update((unsigned char*)prev, prev_len);
    }

    fw_get_firmware_buff.buffer = data;
    fw_get_firmware_buff.size   = size;

    /* following loop breaks out after getting the firmware_response */
    rc = greybus_loop();
    if (rc) {
        dbgprintx32("FW receive failed: -", -rc, "\n");
        return rc;
    }
    if (responded_op != (GB_BOOT_OP_GET_FIRMWARE | GB_TYPE_RESPONSE)) {
        dbgprint("Response wasn't get-FW\n");
        return -ENODEV;
    }

    return 0;
}

static int gbboot_get_firmware_response(gb_operation_header *header, void *data,
                                      uint32_t len) {
    if (header->status) {
        dbgprint("gbboot_get_firmware_response(): err status\n");
        return -header->status;
    }
    if (header->size - sizeof(gb_operation_header) != len ||
        len != fw_get_firmware_buff.size) {
        dbgprint("gbboot_get_firmware_response(): wrong response size\n");
        return GB_BOOT_ERR_INVALID;
    }
    memcpy(fw_get_firmware_buff.buffer, data, fw_get_firmware_buff.size);
    /* return >0 to break out from greybus loop */
    return 1;
}

static int gbboot_ready_to_boot(uint8_t status) {
    int rc;
    struct gbboot_ready_to_boot_request req = {status};
    rc = greybus_send_request(gbboot_cportid, 1, GB_BOOT_OP_READY_TO_BOOT,
                              (uint8_t*)&req, sizeof(req));
    if (rc) {
        return rc;
    }

    /* following loop breaks out after getting the ready_to_boot_response */
    rc = greybus_loop();
    if (rc) {
        return rc;
    }

    return 0;
}

static int gbboot_ready_to_boot_response(gb_operation_header *header, void *data,
                                       uint32_t len) {
    if (header->status) {
        dbgprint("gbboot_ready_to_boot_response(): err status\n");
        return -header->status;
    }
    /* return >0 to break out from greybus loop */
    return 1;
}

static int firmware_size_response_handler(uint32_t cportid,
                                          gb_operation_header *op_header) {
    void *data;
    size_t len;

    data = (void *)(((uint8_t *)op_header) + sizeof(gb_operation_header));
    len = op_header->size - sizeof(gb_operation_header);
    responded_op = op_header->type;
    return gbboot_firmware_size_response(op_header, data, len);
}

static int get_firmware_response_handler(uint32_t cportid,
                                         gb_operation_header *op_header) {
    void *data;
    size_t len;

    data = (void *)(((uint8_t *)op_header) + sizeof(gb_operation_header));
    len = op_header->size - sizeof(gb_operation_header);
    responded_op = op_header->type;
    return gbboot_get_firmware_response(op_header, data, len);
}

static int ready_to_boot_response_handler(uint32_t cportid,
                                          gb_operation_header *op_header) {
    void *data;
    size_t len;

    data = (void *)(((uint8_t *)op_header) + sizeof(gb_operation_header));
    len = op_header->size - sizeof(gb_operation_header);
    responded_op = op_header->type;
    return gbboot_ready_to_boot_response(op_header, data, len);
}

static greybus_op_handler gbboot_cport_handlers[] = {
    {GB_BOOT_OP_PROTOCOL_VERSION, gbboot_get_version},
    {GB_BOOT_OP_FIRMWARE_SIZE | GB_TYPE_RESPONSE,
        firmware_size_response_handler},
    {GB_BOOT_OP_GET_FIRMWARE | GB_TYPE_RESPONSE,
        get_firmware_response_handler},
    {GB_BOOT_OP_READY_TO_BOOT | GB_TYPE_RESPONSE,
        ready_to_boot_response_handler},
    {GB_BOOT_OP_AP_READY,gbboot_ap_ready},
    {GB_BOOT_OP_GET_VID_PID, gbboot_get_vid_pid},
    {HANDLER_TABLE_END, NULL}
};

static int offset = -1;
static uint32_t firmware_size = 0;

static int data_load_greybus_init(void) {
    int rc;

    offset = 0;

    greybus_register_handlers(GBBOOT_CPORT, gbboot_cport_handlers);

    rc = greybus_loop();
    if (rc) {
        set_last_error(BRE_BOU_GBBOOT_CPORT);
        return rc;
    }

    /**
     * Above loop would break out after AP_READY
     * Fetch the firmware size.
     */
    rc = gbboot_firmware_size(NEXT_BOOT_STAGE, &firmware_size);
    if (rc) {
        set_last_error(BRE_BOU_GBBOOT_FW_SIZE);
        goto protocol_error;
    }
    if (firmware_size > WORKRAM_SIZE) {
        rc = -EINVAL;
        set_last_error(BRE_BOU_GBBOOT_FW_TOO_LARGE);
        goto protocol_error;
    }

    return 0;

protocol_error:
    return rc;
}

static int data_load_greybus_load(void *dest, uint32_t length, bool hash) {
    int rc;
    uint32_t blk_len, prev_len = 0;
    void *prev = NULL;
    if (offset + length > firmware_size) {
        return GB_BOOT_ERR_INVALID;
    }

    while (length) {
        /**
         * We take whichever is smaller: the largest possible size for a Greybus
         * message payload, or the remaining length of the firmware blob.
         */
        blk_len = (length > GB_MAX_PAYLOAD_SIZE) ? GB_MAX_PAYLOAD_SIZE : length;
        rc = gbboot_get_firmware(offset, blk_len, dest, prev, prev_len);
        if (rc) {
            set_last_error(BRE_BOU_GBBOOT_GET_FW);
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
    uint8_t status = GB_BOOT_BOOT_STATUS_INVALID;
    if (valid && is_secure_image) {
        status = GB_BOOT_BOOT_STATUS_SECURE;
    }
    else if (valid) {
        status = GB_BOOT_BOOT_STATUS_INSECURE;
    }

    rc = gbboot_ready_to_boot(status);
    if (rc) {
        set_last_error(BRE_BOU_GBBOOT_READY);
    }

    dbgprint("Finished Greybus FW download\n");

    firmware_size = offset = -1;
    return rc;
}

data_load_ops greybus_ops = {
    .init = data_load_greybus_init,
    .read = NULL,
    .load = data_load_greybus_load,
    .finish = data_load_greybus_finish
};
