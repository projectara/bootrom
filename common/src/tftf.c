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
#include "bootrom.h"
#include "tftf.h"
#include "debug.h"
#include "data_loading.h"
#include "chipapi.h"
#include "crypto.h"

typedef struct {
    tftf_header header;
    crypto_processing_state crypto_state;
    unsigned char hash[HASH_DIGEST_SIZE];
    tftf_signature signature;
} tftf_processing_state;

static tftf_processing_state tftf;
static const char tftf_sentinel_value[] = TFTF_SENTINEL_VALUE;

static int load_tftf_header(data_load_ops *ops) {
    tftf_section_descriptor *section;
    uint32_t *psentinel = (uint32_t *)&tftf_sentinel_value[0];

    tftf.crypto_state = CRYPTO_STATE_INIT;

    if (ops->load(&tftf.header, TFTF_HEADER_SIZE)) {
        return -1;
    }
    /* first check the sentinel value */
    if (tftf.header.sentinel_value != *psentinel) {
        dbgprint("invalid tftf sentinel value\r\n");
        return -1;
    }

    if (tftf.header.expanded_length < tftf.header.load_length ||
        chip_validate_data_load_location((void *)tftf.header.load_base,
                                         tftf.header.expanded_length)) {
        dbgprint("out of memory range\r\n");
        return -1;
    }

    section = &tftf.header.sections[0];
    while(1) {
        if ((uint32_t)section - (uint32_t)&tftf.header >= TFTF_HEADER_SIZE) {
            dbgprint("too many tftf sections?\r\n");
            return -1;
        }

        if (section->section_type == TFTF_SECTION_END) {
            break;
        }

        if (section->section_type == TFTF_SECTION_SIGNATURE) {
            if (tftf.crypto_state == CRYPTO_STATE_INIT) {
                uint32_t header_hash_len;

                /* found the first signature block, start hashing */
                hash_start();
                tftf.crypto_state = CRYPTO_STATE_HASHING;
                header_hash_len = (uint32_t)section - (uint32_t)&tftf.header;
                hash_update((unsigned char *)&tftf.header, header_hash_len);
            }
        } else {
            if (section->section_type != TFTF_SECTION_RAW_CODE &&
                section->section_type != TFTF_SECTION_RAW_DATA &&
                section->section_type != TFTF_SECTION_COMPRESSED_CODE &&
                section->section_type != TFTF_SECTION_COMPRESSED_DATA &&
                section->section_type != TFTF_SECTION_MANIFEST &&
                section->section_type != TFTF_SECTION_CERTIFICATE) {
                dbgprint("Invalid section type found\r\n");
                return -1;
            }

            if (tftf.crypto_state == CRYPTO_STATE_HASHING &&
                section->section_type != TFTF_SECTION_CERTIFICATE) {
                dbgprint("ilegal section after first signature\r\n");
                return -1;
            }

            uint32_t sec_top = section->copy_offset + section->expanded_length;
            if (section->expanded_length < section->section_length ||
                sec_top > tftf.header.expanded_length) {
                dbgprint("section out of memory range\r\n");
                return -1;
            }
        }
        section++;
    }

    /* the header is validated */
    return 0;
}

static int process_tftf_section(data_load_ops *ops,
                                tftf_section_descriptor *section) {
    unsigned char *dest;

    if (section->section_type == TFTF_SECTION_SIGNATURE) {
        dbgprint("WARNING: signature verification not implemented yet\r\n");
        if (ops->load(&tftf.signature, sizeof(tftf.signature))) {
            dbgprint("error loading signature\r\n");
            return -1;
        }
        switch (tftf.crypto_state) {
        case CRYPTO_STATE_HASHING:
            hash_final(tftf.hash);
            tftf.crypto_state = CRYPTO_STATE_HASHED;
            /* fall through */
        case CRYPTO_STATE_HASHED:
            if (verify_signature(tftf.hash, &tftf.signature) == 0) {
                tftf.crypto_state = CRYPTO_STATE_VERIFIED;
            }
            break;
        default:
            /* For other state, nothing need to be done here */
            break;
        }
        return 0;
    }

    dest = (unsigned char*)tftf.header.load_base + section->copy_offset;

    if (section->section_type == TFTF_SECTION_COMPRESSED_CODE ||
        section->section_type == TFTF_SECTION_COMPRESSED_DATA) {
        /* for compressed section, copy the compressed data to the end of
           the section, so in-place decompression would be easier */
        dest += section->expanded_length - section->section_length;
    }

    if (ops->load(dest, section->section_length)) {
        dbgprint("invalid tftf header size\r\n");
        return -1;
    }

    if (tftf.crypto_state == CRYPTO_STATE_HASHING) {
        hash_update(dest, section->section_length);
    }

    if (section->section_type == TFTF_SECTION_COMPRESSED_CODE ||
        section->section_type == TFTF_SECTION_COMPRESSED_DATA) {
        dbgprint("WARNING: decompression not implemented yet\r\n");
        /* TBD: decompress section */
    }
    return 0;
}

int load_tftf_image(data_load_ops *ops, uint32_t *is_secure_image) {
    tftf_section_descriptor *section;

    *is_secure_image = 0;

    if (load_tftf_header(ops)) {
        return -1;
    }

    section = &tftf.header.sections[0];
    while(section->section_type != TFTF_SECTION_END) {
        if (process_tftf_section(ops, section)) {
            return -1;
        }
        section++;
    }

    if (tftf.crypto_state == CRYPTO_STATE_VERIFIED) {
        /* finished loading and verifying secured image */
        *is_secure_image = 1;
    }

    communication_area *p = (communication_area *)&_communication_area;
    memcpy(p->stage_2_firmware_description,
           tftf.header.build_timestamp,
           sizeof(p->stage_2_firmware_description));

    return 0;
}

void jump_to_image(void) {
    dbgflush();
    chip_jump_to_image(tftf.header.start_location);
}
