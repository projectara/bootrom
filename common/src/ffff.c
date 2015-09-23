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
#include <stdint.h>
#include <stdbool.h>
#include "ffff.h"
#include "utils.h"
#include "debug.h"
#include "data_loading.h"
#include "error.h"

typedef struct {
    ffff_header header1;
    ffff_header header2;
    ffff_header *cur_header;
    ffff_element_descriptor *cur_element;
} ffff_processing_state;

static ffff_processing_state ffff;
static const char ffff_sentinel_value[] = FFFF_SENTINEL_VALUE;

/**
 * @brief Validate an FFFF element
 *
 * @param section The FFFF element descriptor to validate
 * @param header (The RAM-address of) the FFFF header to which it belongs
 * @param rom_address The address in SPIROM of the header
 * @param end_of_elements Pointer to a flag that will be set if the element
 *        type is the FFFF_ELEMENT_END marker. (Untouched if not)
 *
 * @returns True if valid element, false otherwise
 */
bool valid_ffff_element(ffff_element_descriptor * element,
                        ffff_header * header, uint32_t rom_address,
                        bool *end_of_elements) {
    ffff_element_descriptor * other_element;
    uint32_t element_location_min = rom_address + header->erase_block_size;
    uint32_t element_location_max = header->flash_image_length;
    uint32_t this_start;
    uint32_t this_end;
    uint32_t that_start;
    uint32_t that_end;

    /*
     * Because we don't *know* the real length of the Flash, it is possible to
     * read past the end of the flash (which simply wraps around). This is benign
     * because the resulting mess will not validate.
     */

    /* Is this the end-of-table marker? */
    if (element->element_type == FFFF_ELEMENT_END) {
        *end_of_elements = true;
        return true;
    }

    /* Do we overlap the header or spill over the end? */
    this_start = element->element_location;
    this_end = this_start + element->element_length - 1;
    if ((this_start < element_location_min) ||
        (this_end >= element_location_max)) {
        set_last_error(BRE_FFFF_ELT_RESERVED_MEMORY);
        return false;
    }

    /* Are we block-aligned? */
    if (!block_aligned(element->element_location, header->erase_block_size)) {
        set_last_error(BRE_FFFF_ELT_ALIGNMENT);
        return false;
    }

    /*
     * Check this element for:
     *   a) collisions against all following sections and
     *   b) duplicates of this element.
     * Since we're called in a scanning fashion from the start to the end of
     * the elements array, all elements before us have already validated that
     * they don't duplicate or collide with us.
     */
    for (other_element = element + 1;
         ((other_element < &header->elements[FFFF_MAX_ELEMENTS]) &&
          (other_element->element_type != FFFF_ELEMENT_END));
         other_element++) {
        /* (a) check for collision */
        that_start = other_element->element_location;
        that_end = that_start + other_element->element_length - 1;
        if ((that_end >= this_start) && (that_start <= this_end)) {
            set_last_error(BRE_FFFF_ELT_COLLISION);
            return false;
        }


        /*
         * (b) Check for  duplicate entries per the specification:
         * "At most, one element table entry with a particular element type,
         * element ID, and element generation may be present in the element
         * table."
         */
        if ((element->element_type == other_element->element_type) &&
            (element->element_id == other_element->element_id) &&
            (element->element_generation ==
                    other_element->element_generation)) {
            set_last_error(BRE_FFFF_ELT_DUPLICATE);
            return false;
        }
    }

    /* No errors found! */
    return true;
}

static int validate_ffff_header(ffff_header *header, uint32_t address) {
    int i;
    ffff_element_descriptor * element;
    bool end_of_elements = false;

    /* Check for leading and trailing sentinels */
    for (i = 0; i < FFFF_SENTINEL_SIZE; i++) {
        if (header->sentinel_value[i] != ffff_sentinel_value[i]) {
            set_last_error(BRE_FFFF_SENTINEL);
            return -1;
        }
    }
    for (i = 0; i < FFFF_SENTINEL_SIZE; i++) {
        if (header->trailing_sentinel_value[i] != ffff_sentinel_value[i]) {
            set_last_error(BRE_FFFF_SENTINEL);
            return -1;
        }
    }

    if (header->erase_block_size > FFFF_ERASE_BLOCK_SIZE_MAX) {
        set_last_error(BRE_FFFF_BLOCK_SIZE);
        return -1;
    }

    if (header->flash_capacity < (header->erase_block_size << 1)) {
        set_last_error(BRE_FFFF_FLASH_CAPACITY);
       return -1;
    }

    if (header->flash_image_length > header->flash_capacity) {
        set_last_error(BRE_FFFF_IMAGE_LENGTH);
        return -1;
    }

    if (header->header_size != FFFF_HEADER_SIZE) {
        set_last_error(BRE_FFFF_HEADER_SIZE);
       return -1;
    }

    /* Validate the FFFF elements */
    for (element = &header->elements[0];
         (element < &header->elements[FFFF_MAX_ELEMENTS]) && !end_of_elements;
         element++) {
        if (!valid_ffff_element(element, header, address, &end_of_elements)) {
            /* (valid_ffff_element took care of error reporting) */
            return -1;
        }
    }
    if (!end_of_elements) {
        set_last_error(BRE_FFFF_NO_TABLE_END);
        return -1;
    }

    /*
     * Verify that the remainder of the header (i.e., unused section
     * descriptors and the padding) is zero-filled
     */
    if (!is_constant_fill((uint8_t *)element,
                          (uint32_t)&header->trailing_sentinel_value -
                              (uint32_t)element,
                          0x00)) {
        set_last_error(BRE_FFFF_NON_ZERO_PAD);
        return -1;
    }

    /* No errors found! */
    return 0;
}

static int locate_ffff_table(data_load_ops *ops)
{
    uint32_t address = 0;

    ffff.cur_header = &ffff.header1;

    /*** TODO: Fix wording */
    /* First look for header at beginning of the storage */
    if (ops->read(&ffff.header1, address, sizeof(ffff_header))) {
        set_last_error(BRE_FFFF_LOAD_HEADER);
        return -1;
    } else if (validate_ffff_header(&ffff.header1, address)) {
        /* There is no valid FFFF table at address 0, this means the first
           copy of FFFF table is corrupted. So look for the second copy only */
        address = FFFF_HEADER_SIZE;
        while(address < FFFF_ERASE_BLOCK_SIZE_MAX * 2) {
            if (ops->read(&ffff.header2, address, sizeof(ffff_header))) {
                set_last_error(BRE_FFFF_LOAD_HEADER);
                return -1;
            } else if(!validate_ffff_header(&ffff.header2, address)) {
                ffff.cur_header = &ffff.header2;
                reset_last_error();
                return 0;
            }
            address <<= 1;
        }
        set_last_error(BRE_FFFF_HEADER_NOT_FOUND);
        return -1;
    }

    /* A valid FFFF table is at address 0, now look for the second one */
    address = ffff.header1.erase_block_size;
    if (address < ffff.header1.header_size) {
        address = ffff.header1.header_size;
    }
    if (ops->read(&ffff.header2, address, sizeof(ffff_header))) {
        set_last_error(BRE_FFFF_LOAD_HEADER);
        return -1;
    } else if(validate_ffff_header(&ffff.header2, address)) {
        /* Did not find the second copy, so use the first one */
        reset_last_error();
        return 0;
    }

    if (ffff.header2.header_generation > ffff.header1.header_generation) {
        ffff.cur_header = &ffff.header2;
    }
    reset_last_error();
    return 0;
}

static int locate_element(data_load_ops *ops,
                          uint32_t type,
                          uint32_t *length) {
    uint32_t last_possible_element = (uint32_t)ffff.cur_header +
                                     ffff.cur_header->header_size -
                                     FFFF_SENTINEL_SIZE -
                                     sizeof(ffff_element_descriptor);

    if (length != NULL) {
        *length = 0;
    }

    ffff_element_descriptor *element = &ffff.cur_header->elements[0];

    ffff.cur_element = NULL;

    while ((uint32_t)element <= last_possible_element) {
        if (element->element_type == FFFF_ELEMENT_END) {
            break;
        }

        if (element->element_type == type) {
            if (ffff.cur_element == NULL ||
                ffff.cur_element->element_generation <
                element->element_generation) {
                ffff.cur_element = element;
            }
        }
        element++;
    }

    if (ffff.cur_element == NULL) {
        set_last_error(BRE_FFFF_NO_FIRMWARE);
        return -1;
    }

    /*** TODO: Eliminate this validation or merge into validation suite */
    /* validate the element */
    if (ffff.cur_element->element_location <
        (ffff.cur_header->header_size << 1)) {
        set_last_error(BRE_FFFF_MEMORY_RANGE);
        return -1;
    }
    if (ffff.cur_element->element_location + ffff.cur_element->element_length >
        ffff.cur_header->flash_image_length) {
        set_last_error(BRE_FFFF_MEMORY_RANGE);
        return -1;
    }

    if (length != NULL) {
        *length = ffff.cur_element->element_length;
    }
    return 0;
}

int locate_ffff_element_on_storage(data_load_ops *ops,
                                   uint32_t type,
                                   uint32_t *length) {
    if (ops->read == NULL) {
        set_last_error(BRE_FFFF_NO_FIRMWARE); /*** TODO: Change to BRE_FFFF_LOGIC_ERROR */
        return -1;
    }

    if (locate_ffff_table(ops)) {
        /* (locate_ffff_table took care of error reporting) */
        return -1;
    }

    if (locate_element(ops, type, length)) {
        /* (locate_next_stage_firmware took care of error reporting) */
        return -1;
    }

    /* found the second stage firmware, set the address for the next
       ops->load function to load data from */
    ops->read(NULL, ffff.cur_element->element_location, 0);
    return 0;
}
