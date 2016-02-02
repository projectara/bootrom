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

#ifndef __COMMON_INCLUDE_2NDSTAGE_CFGDATA_H
#define __COMMON_INCLUDE_2NDSTAGE_CFGDATA_H

#include <stdint.h>
#include <crypto.h>

#define S2LCFG_MAX_SIZE 1024

#define SECONDSTAGE_CFG_SENTINEL_SIZE 16
/**
 * config data sentinel value. It is a 16bytes char array. The trailing '\0'
 * in the string is not used.
 */
static const char secondstage_cfg_sentinel[] = "2ndStageFWConfig";
/* Compile-time test to verify consistency of size and value */
typedef char ___secondcfg_sentinel_test[(SECONDSTAGE_CFG_SENTINEL_SIZE ==
                                      sizeof(secondstage_cfg_sentinel) - 1) ?
                                     1 : 0];

/**
 * There are 35 bytes of IMS in TSB chip.
 * Fake ims is for developer debugging only.
 */
#define FAKE_IMS_SIZE 35

typedef struct {
    char sentinel[SECONDSTAGE_CFG_SENTINEL_SIZE];
    uint32_t disable_jtag;
    uint32_t use_fake_ara_vidpid;
    uint32_t fake_ara_vid;
    uint32_t fake_ara_pid;

    uint8_t  use_fake_ims;
    uint8_t  fake_ims[FAKE_IMS_SIZE];

    uint32_t number_of_public_keys;
    crypto_public_key public_keys[0];
} __attribute__ ((packed)) secondstage_cfgdata;

/**
 * @brief get pointer for second stage config data
 * @param cfgdata pointer to the config data
 * @return 0 valid config data found
 *         <0 no valid config data
 */
int get_2ndstage_cfgdata(secondstage_cfgdata **cfgdata);

#endif /* __COMMON_INCLUDE_2NDSTAGE_CFGDATA_H */

