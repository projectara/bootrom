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

#ifndef __COMMON_INCLUDE_FFFF_H
#define __COMMON_INCLUDE_FFFF_H

#include <stdint.h>
#include "data_loading.h"

#define FFFF_HEADER_SIZE                  512
#define FFFF_ERASE_BLOCK_SIZE_MAX         (1024 * 512)

#define FFFF_SENTINEL_SIZE                16
#define FFFF_SENTINEL_VALUE               "FlashFormatForFW"

#define FFFF_MAX_ELEMENTS                 19
#define FFFF_PADDING                      16

/* Element types */
#define FFFF_ELEMENT_END                  0
#define FFFF_ELEMENT_STAGE_2_FW           1
#define FFFF_ELEMENT_STAGE_3_FW           2
#define FFFF_ELEMENT_IMS_CERT             3
#define FFFF_ELEMENT_CMS_CERT             4
#define FFFF_ELEMENT_DATA                 5

typedef struct {
    uint32_t element_type;
    uint32_t element_id;
    uint32_t element_generation;
    uint32_t element_location;
    uint32_t element_length;
} __attribute__ ((packed)) ffff_element_descriptor;

#define FFFF_ELEMENT_SIZE sizeof(ffff_element_descriptor)

typedef //union {
    struct {
        char sentinel_value[FFFF_SENTINEL_SIZE];
        char build_timestamp[16];
        char flash_image_name[48];
        uint32_t flash_capacity;
        uint32_t erase_block_size;
        uint32_t header_size;
        uint32_t flash_image_length;
        uint32_t header_generation;
        ffff_element_descriptor elements[FFFF_MAX_ELEMENTS];
        uint8_t padding[FFFF_PADDING];
        char trailing_sentinel_value[FFFF_SENTINEL_SIZE];
//    };
//    struct {
//        unsigned char leading_buffer[FFFF_HEADER_SIZE - FFFF_SENTINEL_SIZE];
//        char trailing_sentinel_value[FFFF_SENTINEL_SIZE];
//    };
} __attribute__ ((packed)) ffff_header;

int locate_second_stage_firmware_on_storage(data_load_ops *ops);
#endif /* __COMMON_INCLUDE_FFFF_H */
