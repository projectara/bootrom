/*
 * Copyright (c) 2015 Google, Inc.
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
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "appcfg.h"
#include "chip.h"
#include "chipdef.h"
#include "debug.h"
#include "greybus.h"
#include "spi-gb.h"
#include "nuttx_dev_if.h"
#include "device_spi.h"

#define GB_SPI_VERSION_MAJOR 0
#define GB_SPI_VERSION_MINOR 1

static struct device local_dev;
static struct device *spi_dev = &local_dev;

static int gb_spi_get_version(uint32_t cportid,
                              gb_operation_header *op_header) {
    struct gb_spi_proto_version_response payload = {GB_SPI_VERSION_MAJOR,
                                                    GB_SPI_VERSION_MINOR};

    return greybus_op_response(cportid,
                               op_header,
                               GB_OP_SUCCESS,
                               (unsigned char *)&payload,
                               sizeof(payload));
}

static int gb_spi_type_master_config(uint32_t cportid,
                                     gb_operation_header *op_header) {
    struct gb_spi_master_config_response payload;
    struct master_spi_caps caps;

    /* get hardware capabilities */
    if (device_spi_get_master_caps(spi_dev, &caps)) {
        greybus_op_response(cportid,
                            op_header,
                            GB_OP_UNKNOWN_ERROR,
                            NULL,
                            0);
        return -1;
    }

    payload.bpw_mask = cpu_to_le32(caps.bpw);
    payload.min_speed_hz = cpu_to_le32(caps.min_speed_hz);
    payload.max_speed_hz = cpu_to_le32(caps.max_speed_hz);
    payload.mode = cpu_to_le16(caps.modes);
    payload.flags = cpu_to_le16(caps.flags);
    payload.num_chipselect = cpu_to_le16(caps.csnum);

    return greybus_op_response(cportid,
                               op_header,
                               GB_OP_SUCCESS,
                               (unsigned char *)&payload,
                               sizeof(payload));
}

static int gb_spi_type_device_config(uint32_t cportid,
                                     gb_operation_header *op_header) {
    struct gb_spi_device_config_request *request;
    struct gb_spi_device_config_response payload;
    struct device_spi_cfg dev_cfg;
    uint8_t cs;
    int ret = 0;

    if (gb_operation_request_size(op_header) < sizeof(*request)) {
        greybus_op_response(cportid,
                            op_header,
                            GB_OP_INVALID,
                            NULL,
                            0);
        return -1;
    }

    request = (struct gb_spi_device_config_request *)
                (((uint8_t *)op_header) +
                 sizeof(gb_operation_header));
    cs = request->chip_select;

    /* get selected chip of configuration */
    ret = device_spi_get_device_cfg(spi_dev, cs, &dev_cfg);
    if (ret) {
        greybus_op_response(cportid,
                            op_header,
                            GB_OP_UNKNOWN_ERROR,
                            NULL,
                            0);
        return -1;
    }

    payload.mode = cpu_to_le16(dev_cfg.mode);
    payload.bpw = cpu_to_le32(dev_cfg.bpw);
    payload.max_speed_hz = cpu_to_le32(dev_cfg.max_speed_hz);
    payload.device_type = dev_cfg.device_type;
    memcpy(payload.name, dev_cfg.name, sizeof(dev_cfg.name));
    return greybus_op_response(cportid,
                               op_header,
                               GB_OP_SUCCESS,
                               (unsigned char *)&payload,
                               sizeof(payload));
}

static int gb_spi_transfer(uint32_t cportid, gb_operation_header *op_header) {
    struct gb_spi_transfer_desc *desc;
    struct gb_spi_transfer_request *request;
    struct device_spi_transfer transfer;
    uint32_t size = 0, freq = 0;
    uint8_t *write_data, *read_buf;
    bool selected = false;
    int i, op_count;
    int ret = 0;
    struct gb_spi_transfer_response *response;
    size_t expected_size;
    size_t request_size;

    request_size = gb_operation_request_size(op_header);
    if (request_size < sizeof(*request)) {
        greybus_op_response(cportid,
                            op_header,
                            GB_OP_INVALID,
                            NULL,
                            0);
        return -1;
    }
    request = (struct gb_spi_transfer_request *)
                (((uint8_t *)op_header) +
                 sizeof(gb_operation_header));

    op_count = le16_to_cpu(request->count);

    expected_size = sizeof(*request) +
                    op_count * sizeof(request->transfers[0]);
    if (request_size < expected_size) {
        greybus_op_response(cportid,
                            op_header,
                            GB_OP_INVALID,
                            NULL,
                            0);
        return -1;
    }

    write_data = (uint8_t *)&request->transfers[op_count];

    for (i = 0; i < op_count; i++) {
        desc = &request->transfers[i];
        if (desc->rdwr & SPI_XFER_READ) {
            size += le32_to_cpu(desc->len);
        }
    }

    unsigned char payload[size];
    response = (struct gb_spi_transfer_response *)payload;
    read_buf = response->data;

    /* set SPI mode */
    ret = device_spi_setmode(spi_dev, request->chip_select, request->mode);
    if (ret) {
        goto spi_err;
    }

    /* parse all transfer request from AP host side */
    for (i = 0; i < op_count; i++) {
        desc = &request->transfers[i];
        freq = le32_to_cpu(desc->speed_hz);

        /* set SPI bits-per-word */
        ret = device_spi_setbits(spi_dev, request->chip_select,
                                 desc->bits_per_word);
        if (ret) {
            goto spi_err;
        }

        /* set SPI clock */
        ret = device_spi_setfrequency(spi_dev, request->chip_select, &freq);
        if (ret) {
            goto spi_err;
        }

        /* assert chip-select pin */
        if (!selected) {
            ret = device_spi_select(spi_dev, request->chip_select);
            if (ret) {
                goto spi_err;
            }
            selected = true;
        }

        /* setup SPI transfer */
        memset(&transfer, 0, sizeof(struct device_spi_transfer));
        if (desc->rdwr & SPI_XFER_WRITE) {
            transfer.txbuffer = write_data;
        } else {
            transfer.txbuffer = NULL;
        }

        if (desc->rdwr & SPI_XFER_READ) {
            transfer.rxbuffer = read_buf;
        } else {
            transfer.rxbuffer = NULL;
        }

        transfer.nwords = le32_to_cpu(desc->len);

        /* start SPI transfer */
        ret = device_spi_exchange(spi_dev, &transfer);
        if (ret) {
            goto spi_err;
        }
        /* move to next gb_spi_transfer data buffer */
        if (desc->rdwr & SPI_XFER_WRITE) {
            write_data += le32_to_cpu(desc->len);
        }

        /* If rdwr without SPI_XFER_READ flag, not need to resize
         * read buffer
         */
        if (desc->rdwr & SPI_XFER_READ) {
            read_buf += le32_to_cpu(desc->len);
        }

        /* if cs_change enable, change the chip-select pin signal */
        if (desc->cs_change) {
            /* force deassert chip-select pin */
            ret = device_spi_deselect(spi_dev, request->chip_select);
            if (ret) {
                goto spi_err;
            }
            selected = false;
        }

        if (le16_to_cpu(desc->delay_usecs) > 0) {
            delay_ns(le16_to_cpu(desc->delay_usecs) * 1000);
        }
    }

spi_err:
    if (selected) {
        /* deassert chip-select pin */
        ret = device_spi_deselect(spi_dev, request->chip_select);
        if (ret) {
            greybus_op_response(cportid,
                            op_header,
                            GB_OP_UNKNOWN_ERROR,
                            NULL,
                            0);
            return -1;
        }
    }
    return greybus_op_response(cportid,
                              op_header,
                              GB_OP_SUCCESS,
                              (unsigned char *)response,
                              size);
}

static greybus_op_handler spi_gb_cport_handlers[] = {
    {GB_SPI_PROTOCOL_VERSION, gb_spi_get_version},
    {GB_SPI_TYPE_MASTER_CONFIG, gb_spi_type_master_config},
    {GB_SPI_TYPE_DEVICE_CONFIG, gb_spi_type_device_config},
    {GB_SPI_PROTOCOL_TRANSFER, gb_spi_transfer},
    {HANDLER_TABLE_END, NULL}
};

void retister_spi_device(void *ops) {
    spi_dev->ops = ops;
}

void spi_gb_init(void) {
    greybus_register_handlers(GB_SPI_CPORT, spi_gb_cport_handlers);
}
