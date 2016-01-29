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
#include <string.h>
#include <errno.h>
#include "bootrom.h"
#include "debug.h"
#include "crypto.h"
#include "mcl_arch.h"
#include "mcl_config.h"
#include "mcl_rsa_runtime.h"
#include "mcl_ecdh_runtime.h"
#include "secret_keys.h"
#include "communication_area.h"

#define DBG_SECRET_KEY_MSG 0

/* compiler hack to verify array sizes */
typedef char ___epsk_test[(sizeof(((secret_keys_comm_area *)NULL)->epsk) ==
                           MCL_EGS1) ?
                          1 : -1];
typedef char ___essk_test[(sizeof(((secret_keys_comm_area *)NULL)->essk) ==
                           MCL_EGS2) ?
                          1 : -1];
typedef char ___ergs_test[(sizeof(((secret_keys_comm_area *)NULL)->ergs) ==
                           SHA256_HASH_DIGEST_SIZE) ?
                          1 : -1];
typedef char ___essk_test[(sizeof(((secret_keys_comm_area *)NULL)->errk_n) ==
                           RSA2048_PUBLIC_KEY_SIZE) ?
                          1 : -1];

#if DBG_SECRET_KEY_MSG
/**
 * Print out the buffer the same style as imsgen does on the host side
 * so we can compare the results easily.
 */
static void dbgprinthexbuf(uint8_t *buf, int len, char *label) {
    int i;
    for (i = 0; i < len; i++) {
        if ((i & 0x01F) == 0) {
            /* New line every 32 bytes */
            dbgprint("\n");
            dbgprint(label);
        }
        dbgprinthex8(buf[i]);
    }
    dbgprint("\n");
}

void ff_dump(mcl_chunk ff[][MCL_NLEN1], int n, char *label) {
    char local_buf[MCL_MODBYTES1 * n];
    char *p = local_buf;
    int len = MCL_MODBYTES1 * n;

    mcl_octet oct = {0, sizeof(local_buf), local_buf};
    memset(local_buf, 0, sizeof(local_buf));
    MCL_FF_toOctet_C448(&oct, ff, n);

    /* skip the leading zero */
    while (*p == 0) {
        p++;
        len--;
    }
    dbgprinthexbuf((uint8_t*)p, len, label);
}
#endif

static void calculate_y2(uint8_t *ims, uint8_t *y2) {
    /* Y2 = sha256(IMS[0:31] xor copy(0x5a, 32)) */
    uint32_t i;
    uint32_t temp;
    uint32_t *pims = (uint32_t *)ims;

    hash_start();
    /* grab IMS 4bytes at a time and feed that to hash_update */
    for (i = 0; i < 8; i++) {
        temp = pims[i] ^ 0x5a5a5a5a;
        hash_update((unsigned char *)&temp, sizeof(temp));
    }
    hash_final(y2);
}

static void calculate_epsk(uint8_t *y2, uint8_t *epsk) {
    uint8_t z1[SHA256_HASH_DIGEST_SIZE];
    uint8_t t[SHA256_HASH_DIGEST_SIZE];

    /* Z1 = sha256(Y2 || copy(0x01, 32)) */
    sha256_concat(y2, 0x01, 32, z1);

    /* EPSK[0:31] = sha256(Z1 || copy(0x01, 32)) */
    sha256_concat(z1, 0x01, 32, epsk);

    /* EPSK[32:51] = sha256(Z1 || copy(0x02, 32))[0:19] */
    sha256_concat(z1, 0x02, 32, t);
    memcpy(&epsk[32], t, 24);
}

static void calculate_essk(uint8_t *y2, uint8_t *essk) {
    uint8_t z2[SHA256_HASH_DIGEST_SIZE];

    /* Z2 = sha256(Y2 || copy(0x02, 32)) */
    sha256_concat(y2, 0x02, 32, z2);

    /* ESSK[0:31] = sha256(Z2 || copy(0x01, 32)) */
    sha256_concat(z2, 0x01, 32, essk);
}

static void bytes_to_MCL_FF(uint8_t *buf, size_t len,
                            mcl_chunk ff[][MCL_NLEN1], int n) {
    int i;
    char local_buf[MCL_MODBYTES1 * n];
    mcl_octet oct = {sizeof(local_buf), sizeof(local_buf), local_buf};

    /**
     * This function is only locally used, so parameter checking is skipped
     */
    memset(local_buf, 0, sizeof(local_buf));

    for (i = 0; i < len; i++) {
        /**
         * mcl_octet is big endian and aligned to the end of the buffer
         * but our buffer here is little endian.
         */
        local_buf[sizeof(local_buf) - 1 - i] = buf[i];
    }
    MCL_FF_fromOctet_C448(ff, &oct, n);
}

static void MCL_FF_to_bytes(mcl_chunk ff[][MCL_NLEN1], int n,
                            uint8_t *buf, size_t len) {
    int i;
    char local_buf[MCL_MODBYTES1 * n];
    mcl_octet oct = {sizeof(local_buf), sizeof(local_buf), local_buf};

    /**
     * This function is only locally used, so parameter checking is skipped
     */
    MCL_FF_toOctet_C448(&oct, ff, n);

    for (i = 0; i < len; i++) {
        /**
         * mcl_octet is big endian and aligned to the end of the buffer
         * but our buffer here is little endian.
         */
        buf[i] = local_buf[sizeof(local_buf) - 1 - i];
    }
}

#define ERRK_ALIAS_MOD_BITS 3
#define ERRK_ALIAS_MOD_SHIFT 2
static void calculate_errk(uint8_t *y2, uint8_t *ims, uint8_t *errk_n) {
    uint8_t z3[SHA256_HASH_DIGEST_SIZE];
    MCL_rsa_public_key_RSA2048 pub;
    MCL_rsa_private_key_RSA2048 priv;
    uint8_t errk_p[RSA2048_PUBLIC_KEY_SIZE/2];
    uint8_t errk_q[RSA2048_PUBLIC_KEY_SIZE/2];

    /* Z3 = sha256(Y2 || copy(0x03, 32)) */
    sha256_concat(y2, 0x03, 32, z3);

    /**
     * ERRK_P[0:31] = sha256(Z3 || copy(0x01, 32))
     * ERRK_P[32:63] = sha256(Z3 || copy(0x02, 32))
     * ERRK_P[64:95] = sha256(Z3 || copy(0x03, 32))
     * ERRK_P[96:127] = sha256(Z3 || copy(0x04, 32))
     * ERRK_P[0] |= 0x03
     */
    sha256_concat(z3, 0x01, 32, &errk_p[0]);
    sha256_concat(z3, 0x02, 32, &errk_p[32]);
    sha256_concat(z3, 0x03, 32, &errk_p[64]);
    sha256_concat(z3, 0x04, 32, &errk_p[96]);
    errk_p[0] |= ERRK_ALIAS_MOD_BITS;

    bytes_to_MCL_FF(errk_p, RSA2048_PUBLIC_KEY_SIZE/2, priv.p, MCL_FFLEN1/2);

    /**
     * ERRK_Q[0:31] = sha256(Z3 || copy(0x05, 32))
     * ERRK_Q[32:63] = sha256(Z3 || copy(0x06, 32))
     * ERRK_Q[64:95] = sha256(Z3 || copy(0x07, 32))
     * ERRK_Q[96:127] = sha256(Z3 || copy(0x08, 32))
     * ERRK_Q[0] |= 0x03
     */
    sha256_concat(z3, 0x05, 32, &errk_q[0]);
    sha256_concat(z3, 0x06, 32, &errk_q[32]);
    sha256_concat(z3, 0x07, 32, &errk_q[64]);
    sha256_concat(z3, 0x08, 32, &errk_q[96]);
    errk_q[0] |= ERRK_ALIAS_MOD_BITS;

    bytes_to_MCL_FF(errk_q, RSA2048_PUBLIC_KEY_SIZE/2, priv.q, MCL_FFLEN1/2);

    /* ERRK_P += 4 * IMS[34:32] / 4096 */
    /* ERRK_Q += 4 * IMS[34:32] % 4096 */
    uint32_t alias_ims = (*((uint32_t *)&ims[32]) & 0x00FFFFFF);
    MCL_FF_inc_C448(priv.p,
                    (alias_ims >> 12) << ERRK_ALIAS_MOD_SHIFT,
                    MCL_FFLEN1/2);
    MCL_FF_inc_C448(priv.q,
                    (alias_ims & 0xFFF) << ERRK_ALIAS_MOD_SHIFT,
                    MCL_FFLEN1/2);

#if DBG_SECRET_KEY_MSG
    ff_dump(priv.p, MCL_FFLEN1/2, "p ");
    ff_dump(priv.q, MCL_FFLEN1/2, "q ");
#endif

    /* ERPK_MOD = ERRK_Q * ERRK_P */
    MCL_FF_mul_C448(pub.n, priv.p, priv.q, MCL_FFLEN1/2);

#if DBG_SECRET_KEY_MSG
    ff_dump(pub.n, MCL_FFLEN1, "n ");
#endif

    MCL_FF_to_bytes(pub.n,
                    MCL_FFLEN1,
                    errk_n,
                    RSA2048_PUBLIC_KEY_SIZE);
    /* ERPK_E = 65537 */
    pub.e = 65537;

    /* TBD: do we even need the private key on the bridge side? */
    /* ERRK_D = RSA_secret(ERRK_P, ERRK_Q, ERPK_E) */
}

static void calculate_epck(uint8_t *y2, uint8_t *epck) {
    uint8_t z4[SHA256_HASH_DIGEST_SIZE];

    /* Z4 = sha256(Y2 || copy(0x04, 32)) */
    sha256_concat(y2, 0x04, 32, z4);

    /* EPCK = sha256(Z4 || copy(0x01, 32)) */
    sha256_concat(z4, 0x01, 32, epck);
}

static void calculate_ergs(uint8_t *y2, uint8_t *ergs) {
    uint8_t z5[SHA256_HASH_DIGEST_SIZE];

    /* Z5 = sha256(Y2 || copy(0x05, 32)) */
    sha256_concat(y2, 0x05, 32, z5);

    /* ERGS = sha256(Z5 || copy(0x01, 32)) */
    sha256_concat(z5, 0x01, 32, ergs);
}

void initialize_csprng(csprng *RNG) {
    communication_area *pcomm = (communication_area *)&_communication_area;
    secret_keys_comm_area *key_comm = &(pcomm->second_stage.keys);
    /* TBD: also add the HW random number as part of the seed? */
    mcl_octet SEED = {sizeof(key_comm->ergs),
                      sizeof(key_comm->ergs),
                      (char *)key_comm->ergs};

    MCL_CREATE_CSPRNG_C448(RNG, &SEED);
}

int sign_message_with_epsk(uint8_t *message, size_t len,
                           uint8_t *cs, size_t cs_size,
                           uint8_t *ds, size_t ds_size) {
    communication_area *pcomm = (communication_area *)&_communication_area;
    secret_keys_comm_area *key_comm = &(pcomm->second_stage.keys);

    mcl_octet S0 = {MCL_EGS1, MCL_EGS1, (char*)key_comm->epsk};
    mcl_octet M = {len, len, (char *)message};
    mcl_octet CS = {0, cs_size, (char *)cs};
    mcl_octet DS = {0, ds_size, (char *)ds};

    csprng RNG;
    initialize_csprng(&RNG);

    if (cs == NULL || cs_size < MCL_EGS1 ||
        ds == NULL || ds_size < MCL_EGS1) {
        return -EINVAL;
    }

    if (MCL_ECPSP_DSA_C448(MCL_HASH_TYPE_ECC, &RNG, &S0, &M, &CS, &DS)!=0) {
        return -ERANGE;
    }
    return 0;
}

int sign_message_with_essk(uint8_t *message, size_t len,
                           uint8_t *cs, size_t cs_size,
                           uint8_t *ds, size_t ds_size) {
    communication_area *pcomm = (communication_area *)&_communication_area;
    secret_keys_comm_area *key_comm = &(pcomm->second_stage.keys);

    mcl_octet S0 = {MCL_EGS2, MCL_EGS2, (char*)key_comm->essk};
    mcl_octet M = {len, len, (char *)message};
    mcl_octet CS = {0, cs_size, (char *)cs};
    mcl_octet DS = {0, ds_size, (char *)ds};

    csprng RNG;
    initialize_csprng(&RNG);

    if (cs == NULL || cs_size < MCL_EGS2 ||
        ds == NULL || ds_size < MCL_EGS2) {
        return -EINVAL;
    }

    if (MCL_ECPSP_DSA_C25519(MCL_HASH_TYPE_ECC, &RNG, &S0, &M, &CS, &DS)!=0) {
        return -ERANGE;
    }
    return 0;
}

void key_generation(uint8_t *ims) {
    communication_area *pcomm = (communication_area *)&_communication_area;
    secret_keys_comm_area *key_comm = &(pcomm->second_stage.keys);
    uint8_t y2[SHA256_HASH_DIGEST_SIZE];
    uint8_t epck[SHA256_HASH_DIGEST_SIZE];

    calculate_y2(ims, y2);

    calculate_epsk(y2, key_comm->epsk);
    calculate_ergs(y2, key_comm->ergs);
    calculate_essk(y2, key_comm->essk);

#if DBG_SECRET_KEY_MSG
    dbgprinthexbuf(key_comm->epsk, sizeof(key_comm->epsk), "epsk ");
    dbgprinthexbuf(key_comm->essk, sizeof(key_comm->essk), "essk ");
    dbgprinthexbuf(key_comm->ergs, sizeof(key_comm->ergs), "ergs ");
#endif

    calculate_epck(y2, epck);

    /**
     * To re-create the public key from the communication area, use
     *     bytes_to MCL_FF(key_comm->errk_n,
     *                     RSA2048_PUBLIC_KEY_SIZE,
     *                     pub.n,
     *                     MCL_FFLEN1);
     *     pub.e = 65537;
     */
    calculate_errk(y2, ims, key_comm->errk_n);

    dbgprint("secret keys generated\n");
}
