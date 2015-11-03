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
#include <string.h>
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

/**
 * @brief Validate an FFFF element
 *
 * @param section The FFFF element descriptor to validate
 * @param header (The RAM-address of) the FFFF header to which it belongs
 * @param end_of_elements Pointer to a flag that will be set if the element
 *        type is the FFFF_ELEMENT_END marker. (Untouched if not)
 *
 * @returns True if valid element, false otherwise
 */
bool valid_ffff_element(ffff_element_descriptor * element,
                        ffff_header * header,
                        bool *end_of_elements) {
    ffff_element_descriptor * other_element;
    uint32_t element_location_min;
    uint32_t element_location_max = header->flash_image_length;
    uint32_t this_start;
    uint32_t this_end;
    uint32_t that_start;
    uint32_t that_end;

    /* there are two headers in FFFF */
    if (header->erase_block_size < header->header_size) {
        element_location_min = (header->header_size << 1);
    } else {
        element_location_min = (header->erase_block_size << 1);
    }

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
         (!is_element_out_of_range(header, other_element) &&
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

static int validate_ffff_header(ffff_header *header) {
    ffff_element_descriptor * element;
    bool end_of_elements = false;
    char *trailing_sentinel_value;

    trailing_sentinel_value = get_trailing_sentinel_addr(header);

    /* Check for trailing sentinels */
    if (memcmp(trailing_sentinel_value,
               ffff_sentinel_value,
               FFFF_SENTINEL_SIZE)) {
        set_last_error(BRE_FFFF_SENTINEL);
        return -1;
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

    /* Validate the FFFF elements */
    for (element = &header->elements[0];
         !is_element_out_of_range(header, element) && !end_of_elements;
         element++) {
        if (!valid_ffff_element(element, header, &end_of_elements)) {
            /* (valid_ffff_element took care of error reporting) */
            return -1;
        }
    }
    if (!end_of_elements) {
        set_last_error(BRE_FFFF_NO_TABLE_END);
        return -1;
    }

    /* No errors found! */
    return 0;
}

/**
 * @brief try to load FFFF header from ROM at addr
 * @param ops data loading operation structure
 * @param buf buffer to store the header
 * @param addr address on storage media to load the header
 * @param valid_header indicates whether the header at the specified addr
 *                     is valid FFFF header or not.
 * @return -1 for fatal error, the whole operation should fail
 *          0 for able to load something, *valid_header indicates if
 *            it is a valid FFFF header
 */
static int load_ffff_header(data_load_ops *ops,
                            unsigned char *buf,
                            uint32_t addr,
                            bool *valid_header) {
    ffff_header *header = (ffff_header *)buf;
    *valid_header = false;

    if (ops->read(buf, addr, FFFF_HEADER_SIZE_MIN)) {
        set_last_error(BRE_FFFF_LOAD_HEADER);
        return -1;
    }

    /* Check for leading sentinels */
    if (memcmp(header->sentinel_value,
               ffff_sentinel_value,
               FFFF_SENTINEL_SIZE)) {
        set_last_error(BRE_FFFF_SENTINEL);
        return 0;
    }

    if (header->header_size < FFFF_HEADER_SIZE_MIN ||
        header->header_size > MAX_FFFF_HEADER_SIZE_SUPPORTED) {
        set_last_error(BRE_FFFF_HEADER_SIZE);
       return 0;
    }

    if (header->header_size != FFFF_HEADER_SIZE_MIN) {
        if (ops->read(&buf[FFFF_HEADER_SIZE_MIN],
                      addr + FFFF_HEADER_SIZE_MIN,
                      header->header_size - FFFF_HEADER_SIZE_MIN)) {
            set_last_error(BRE_FFFF_LOAD_HEADER);
            return -1;
        }
    }

    if (!validate_ffff_header(header)) {
        *valid_header = true;
    }

    return 0;
}

static int locate_ffff_table(data_load_ops *ops)
{
    uint32_t address = 0;
    bool valid_header;

    ffff.cur_header = &ffff.header1;

    /**
     * This function sets ffff.cur_header to point to the static buffer
     * containing the newest FFFF header structure in the FFFF storage.
     * Each FFFF storage normally has two identical (and consecutive) copies
     * of the FFFF header at the beginning of the storage, but those can
     * become damaged or out of sync if an update operation is interrupted.
     * This code looks for the first header, then looks for a second one.
     * If it finds the first header, that tells it where the second header
     * will be; otherwise, it searches on power-of-two boundaries to locate
     * a second header. It is an error (failure) if no valid headers can be
     * found. If only one is found, it is used as ffff.cur_header. If two are
     * found, the newest one (based on header_generation) is used as
     * ffff.cur_header.
     **/

    /* First look for header at beginning of the storage */
    if (load_ffff_header(ops, ffff.header1.buffer, address, &valid_header)) {
        return -1;
    } else if (!valid_header) {
        /* There is no valid FFFF table at address 0, this means the first
           copy of FFFF table is corrupted. So look for the second copy only */
        address = FFFF_HEADER_SIZE_MIN;
        while(address < FFFF_ERASE_BLOCK_SIZE_MAX * 2) {
            if (load_ffff_header(ops,
                                 ffff.header2.buffer,
                                 address,
                                 &valid_header)) {
                return -1;
            } else if(valid_header) {
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
    if (load_ffff_header(ops, ffff.header2.buffer, address, &valid_header)) {
        return -1;
    } else if(!valid_header) {
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

    if (length != NULL) {
        *length = ffff.cur_element->element_length;
    }
    return 0;
}

int locate_ffff_element_on_storage(data_load_ops *ops,
                                   uint32_t type,
                                   uint32_t *length) {
    if (ops->read == NULL) {
        set_last_error(BRE_FFFF_LOGIC_ERROR);
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
