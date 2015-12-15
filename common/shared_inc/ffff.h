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
#include <stdbool.h>
#include "chipcfg.h"

#define FFFF_HEADER_SIZE_MIN              512
#define FFFF_HEADER_SIZE_MAX              32768

#if MAX_FFFF_HEADER_SIZE_SUPPORTED < FFFF_HEADER_SIZE_MIN || \
    MAX_FFFF_HEADER_SIZE_SUPPORTED > FFFF_HEADER_SIZE_MAX
#error "Invalid MAX_FFFF_HEADER_SIZE_SUPPORTED"
#endif


#define FFFF_ERASE_BLOCK_SIZE_MAX         (1024 * 512)

#define FFFF_SENTINEL_SIZE                16
static const char ffff_sentinel_value[] = "FlashFormatForFW";
/**
 * Compile-time test hack to verify that the sentinel value is
 * FFFF_SENTINEL_SIZE bytes
 */
typedef char ___ffff_sentinel_test[((sizeof(ffff_sentinel_value) -1) ==
                                    FFFF_SENTINEL_SIZE) ?
                                   1 : -1];

#define FFFF_NUM_RESERVED                 4

/* Element types */
#define FFFF_ELEMENT_STAGE_2_FW           0x01
#define FFFF_ELEMENT_STAGE_3_FW           0x02
#define FFFF_ELEMENT_IMS_CERT             0x03
#define FFFF_ELEMENT_CMS_CERT             0x04
#define FFFF_ELEMENT_DATA                 0x05
#define FFFF_ELEMENT_END                  0xfe

typedef struct {
    unsigned int element_type : 8;   /* One of the FFFF_ELEMENT_xxx above */
    unsigned int element_class : 24;
    uint32_t element_id;
    uint32_t element_length;
    uint32_t element_location;
    uint32_t element_generation;
} __attribute__ ((packed)) ffff_element_descriptor;

#define FFFF_ELEMENT_SIZE sizeof(ffff_element_descriptor)
/* Compile-time test hack to verify that the element descriptor is 20 bytes */
typedef char ___ffff_element_test[(FFFF_ELEMENT_SIZE == 20) ? 1 : -1];

typedef union {
    struct __attribute__ ((packed)) {
        char sentinel_value[FFFF_SENTINEL_SIZE];
        char build_timestamp[16];
        char flash_image_name[48];
        uint32_t flash_capacity;
        uint32_t erase_block_size;
        uint32_t header_size;
        uint32_t flash_image_length;
        uint32_t header_generation;
        uint32_t reserved[FFFF_NUM_RESERVED];
        ffff_element_descriptor elements[];
    };
    unsigned char buffer[MAX_FFFF_HEADER_SIZE_SUPPORTED];
} ffff_header;

/**
 * @brief Macro to calculate the last address in an element.
 */
#define ELEMENT_END_ADDRESS(element_ptr) \
    ((element_ptr)->element_load_address + \
     (element_ptr)->element_expanded_length - 1)


/**
 * @brief Macro to calculate the number of elements in an FFFF header
 */
#define CALC_MAX_FFFF_ELEMENTS(header_size) \
		(((header_size) - \
		 (offsetof(ffff_header, elements) + FFFF_SENTINEL_SIZE)) / \
		 sizeof(ffff_element_descriptor))

#define FFFF_HEADER_SIZE_DEFAULT    FFFF_HEADER_SIZE_MAX

static inline char *get_trailing_sentinel_addr(ffff_header *header) {
    return (char *)header->buffer + header->header_size - FFFF_SENTINEL_SIZE;
}

static inline bool is_element_out_of_range(ffff_header *header,
                                           ffff_element_descriptor *element) {
    return ((unsigned char *)element >= header->buffer +
                                        header->header_size -
                                        (FFFF_SENTINEL_SIZE +
                                         sizeof(ffff_element_descriptor)));
}

/**
 * Compile-time test hack to verify that the header is
 * MAX_FFFF_HEADER_SIZE_SUPPORTED bytes
 */
typedef char ___ffff_header_test[(sizeof(ffff_header) ==
                                 MAX_FFFF_HEADER_SIZE_SUPPORTED) ?
                                 1 : -1];

#endif /* __COMMON_INCLUDE_FFFF_H */
