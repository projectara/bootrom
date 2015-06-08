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
#include "tftf.h"
#include "debug.h"
#include "crypto.h"


/**
 * @brief Initialize the SHA hash
 *
 * @param none
 *
 * @returns Nothing
 */
void hash_start(void) {
#ifndef _SIMULATION
    /* TODO: Add hash initialization code once license is signed */
#endif
}


/**
 * @brief Add data to the SHA hash
 *
 * @param data Pointer to the run of data to add to the hash.
 * @param datalen The length in bytes of the data run.
 *
 * @returns Nothing
 */
void hash_update(unsigned char *data, uint32_t datalen) {
#ifndef _SIMULATION
    /* TODO: Add hash update code once license is signed */
#endif
}


/**
 * @brief Finalize the SHA hash and return the digest
 *
 * @param digest Pointer to the digest buffer
 *
 * @returns Nothing
 */
void hash_final(unsigned char *digest) {
#ifndef _SIMULATION
    /* TODO: Add hash finalization code once license is signed */
#endif
}


/**
 * @brief Verify a SHA digest against a signature
 *
 * @param digest The SHA digest obtained from hash-final.
 * @param signature A pointer to the TFTF signature block.
 *
 * @returns 0 if the digest verifies, non-zero otherwise
 */
int verify_signature(unsigned char *digest, tftf_signature *signature) {
#ifdef _SIMULATION
    return 0;
#endif
    /* TODO: Add signature verification code once license is signed */
    return 0;
}
