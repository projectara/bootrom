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
#include "ffff.h"
#include "debug.h"
#include "data_loading.h"

typedef struct {
    ffff_header header1;
    ffff_header header2;
    ffff_header *cur_header;
    ffff_element_descriptor *cur_element;
} ffff_processing_state;

static ffff_processing_state ffff;
static const char ffff_sentinel_value[] = FFFF_SENTINEL_VALUE;

static int validate_ffff_header(ffff_header *header) {
    int i;
    for (i = 0; i < FFFF_SENTINEL_SIZE; i++) {
        if (header->sentinel_value[i] != ffff_sentinel_value[i]) {
            return -1;
        }
    }
    for (i = 0; i < FFFF_SENTINEL_SIZE; i++) {
        if (header->trailing_sentinel_value[i] != ffff_sentinel_value[i]) {
            return -1;
        }
    }

    if (header->erase_block_size > FFFF_ERASE_BLOCK_SIZE_MAX) {
        return -1;
    }

    if (header->flash_capacity < (header->erase_block_size << 1)) {
        return -1;
    }

    if (header->flash_image_length > header->flash_capacity) {
        return -1;
    }

    if (header->header_size != FFFF_HEADER_SIZE) {
        return -1;
    }

    return 0;
}

static int locate_ffff_table(data_load_ops *ops)
{
    uint32_t address = FFFF_HEADER_SIZE;

    ffff.cur_header = &ffff.header1;

    /* First look for header at beginning of the storage */
    if (ops->read(&ffff.header1, 0, sizeof(ffff_header))) {
        return -1;
    } else if (validate_ffff_header(&ffff.header1)) {
        /* There is no valid FFFF table at address 0, this means the first
           copy of FFFF table is corrupted. So look for the second copy only */
        while(address < FFFF_ERASE_BLOCK_SIZE_MAX * 2) {
            if (ops->read(&ffff.header2, address, sizeof(ffff_header))) {
                return -1;
            } else if(!validate_ffff_header(&ffff.header2)) {
                ffff.cur_header = &ffff.header2;
                return 0;
            }
            address <<= 1;
        }
        dbgprint("Failed to locate FFFF table\r\n");
        return -1;
    }

    /* a valid FFFF table is at address 0, now look for the second one */
    address = ffff.header1.erase_block_size;
    if (address < ffff.header1.header_size) {
        address = ffff.header1.header_size;
    }
    if (ops->read(&ffff.header2, address, sizeof(ffff_header))) {
        return -1;
    } else if(validate_ffff_header(&ffff.header2)) {
        /* Did not find the second copy, so use the first one */
        dbgprint("failed to find the second FFFF table\r\n");
        return 0;
    }

    if (ffff.header2.header_generation > ffff.header1.header_generation) {
        ffff.cur_header = &ffff.header2;
    }
    return 0;
}

static int locate_second_stage_firmware(data_load_ops *ops) {
    uint32_t last_possible_element = (uint32_t)ffff.cur_header +
                                     ffff.cur_header->header_size -
                                     FFFF_SENTINEL_SIZE -
                                     sizeof(ffff_element_descriptor);

    ffff_element_descriptor *element = &ffff.cur_header->elements[0];

    ffff.cur_element = NULL;

    while ((uint32_t)element <= last_possible_element) {
        if (element->element_type == FFFF_ELEMENT_END) {
            break;
        }

        if (element->element_type == FFFF_ELEMENT_STAGE_2_FW) {
            if (ffff.cur_element == NULL ||
                ffff.cur_element->element_generation <
                element->element_generation) {
                ffff.cur_element = element;
            }
        }
        element++;
    }

    if (ffff.cur_element == NULL) {
        dbgprint("failed to find the second stage firmware\r\n");
        return -1;
    }

    /* validate the element */
    if (ffff.cur_element->element_location <
        (ffff.cur_header->header_size << 1)) {
        return -1;
    }
    if (ffff.cur_element->element_location + ffff.cur_element->element_length >
        ffff.cur_header->flash_image_length) {
        dbgprint("Second-stage firmware element's location + length exceed \
                  flash-image size.\r\n");
        return -1;
    }

    return 0;
}

int locate_second_stage_firmware_on_storage(data_load_ops *ops) {
    if (ops->read == NULL) {
        return -1;
    }

    if (locate_ffff_table(ops)) {
        return -1;
    }

    if (locate_second_stage_firmware(ops)) {
        return -1;
    }

    /* found the second stage firmware, set the address for the next
       ops->load function to load data from */
    ops->read(NULL, ffff.cur_element->element_location, 0);
    return 0;
}
