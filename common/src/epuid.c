/**
 * Copyright (c) 2016 Google Inc.
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
#include <string.h>
#include "crypto.h"
#include "secret_keys.h"

void calculate_epuid(uint8_t *ims, union large_uint * endpoint_id) {
    unsigned char t[SHA256_HASH_DIGEST_SIZE];
    unsigned char y1[SHA256_HASH_DIGEST_SIZE];
    unsigned char z0[SHA256_HASH_DIGEST_SIZE];
    uint32_t temp;
    uint32_t i;
    uint32_t *pims = (uint32_t *)ims;

    /* Y1 = sha256(IMS[0:15] xor copy(0x3d, 16)) */
    hash_start();
    /* grab IMS 4bytes at a time and feed that to hash_update */
    for (i = 0; i < 4; i++) {
        temp = pims[i] ^ 0x3d3d3d3d;
        hash_update((unsigned char *)&temp, sizeof(temp));
    }
    hash_final(y1);

    /* Z0 = sha256(Y1 || copy(0x01, 32)) */
    sha256_concat(y1, 0x01, 32, z0);

    /* EP_UID[0:7] = sha256(Z0)[0:7] */
    hash_start();
    hash_update(z0, SHA256_HASH_DIGEST_SIZE);
    hash_final(t);
    memcpy(endpoint_id, t, 8);
}

/**
 * Some mistake in ES3 boot ROM makes the EPUID of ES3 different
 * than the spec says
 */
void calculate_es3_epuid(uint8_t *ims, union large_uint * endpoint_id) {
    /* same code used in ES3 boot ROM to generate the EUID */
    int i;
    unsigned char EP_UID[SHA256_HASH_DIGEST_SIZE];
    unsigned char Y1[SHA256_HASH_DIGEST_SIZE];
    unsigned char Z0[SHA256_HASH_DIGEST_SIZE];
    uint32_t temp;
    uint32_t *pims = (uint32_t *)ims;

    hash_start();
    /*** grab IMS 4bytes at a time and feed that to hash_update */
    for (i = 0; i < 4; i++) {
        temp = pims[i] ^ 0x3d3d3d3d;
        hash_update((unsigned char *)&temp, 1);
    }
    hash_final(Y1);

    hash_start();
    hash_update(Y1, SHA256_HASH_DIGEST_SIZE);
    temp = 0x01010101;
    for (i = 0; i < 8; i++) {;
        hash_update((unsigned char *)&temp, 1);
    }
    hash_final(Z0);

    hash_start();
    hash_update(Z0, SHA256_HASH_DIGEST_SIZE);
    hash_final(EP_UID);

    memcpy(endpoint_id, EP_UID, 8);
}

void sha256_concat(uint8_t *input,
                   uint8_t val_to_concat,
                   uint8_t num_to_concat,
                   uint8_t *output) {
    uint32_t i;

    hash_start();
    hash_update(input, SHA256_HASH_DIGEST_SIZE);
    for (i = 0; i < num_to_concat; i++) {;
        hash_update(&val_to_concat, sizeof(val_to_concat));
    }
    hash_final(output);
}
