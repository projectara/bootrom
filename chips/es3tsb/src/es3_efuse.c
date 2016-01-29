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

/*
 * External documents:
 *  - "ARA_ES3_APBridge_RegisterMap_rev050.pdf
 *  - "ARA_ES3_APBridge_AppendixD_rev010.pdf
 *  - "ARA_ES3_APBridge_RegisterMap_AppendixD_rev001.pdf
 */
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "chip.h"
#include "chipapi.h"
#include "debug.h"
#include "tsb_scm.h"
#include "tsb_isaa.h"
#include "bootrom.h"
#include "error.h"
#include "unipro.h"
#include "efuse.h"
#include "crypto.h"
#include "secret_keys.h"

/* Mask values used by "count_ones" */
#define MASK8_1S    ((uint8_t)0x55) /* binary: 0101... */
#define MASK8_2S    ((uint8_t)0x33) /* binary: 00110011.. */
#define MASK8_4S    ((uint8_t)0x0f) /* binary:  4 zeros,  4 ones ... */

#define MASK32_1S   ((uint32_t)0x55555555) /* binary: 0101... */
#define MASK32_2S   ((uint32_t)0x33333333) /* binary: 00110011.. */
#define MASK32_4S   ((uint32_t)0x0f0f0f0f) /* binary:  4 zeros,  4 ones ... */
#define MASK32_8S   ((uint32_t)0x00ff00ff) /* binary:  8 zeros,  8 ones ... */
#define MASK32_16S  ((uint32_t)0x0000ffff) /* binary:  16 zeros, 16 ones ... */

/* IMS is 35 bytes long, but boot ROM only cares about the first 32 bytes */
#define IMS_MEANINGFUL_LENGTH  32

/* Prototypes */
static int count_ones(uint8_t *buf, int len);
static bool valid_hamming_weight(uint8_t *buf, int leni, bool *all_zero);

/**
 * @brief Valdate and publish the e-Fuses as DME attributes
 *
 * @param none
 *
 * @returns 0 on success
 *          -1 on failure
 */
int efuse_init(void) {
    uint32_t    register_val;
    uint32_t    urc;
    union large_uint endpoint_id;
    uint8_t     ims_value[TSB_ISAA_NUM_IMS_BYTES];
    bool all_zero;

    /* Check for e­-Fuse CRC error
     * See ARA_ESx_APBridge_RegisterMap_revxxx.pdf
     * TA-04 Read eFuse status (result ECC)
     */
    register_val = tsb_get_eccerror();
    if ((register_val & TSB_ECCERROR_ECC_ERROR) != 0) {
        dbgprint("Efuse ECC error\n");
        set_last_error(BRE_EFUSE_ECC);
        return -1;
    }


    /* Obtain and verify VID/PID/SN (in e­-Fuse) have proper Hamming weight
     * and advertise various these via DME attribute registers
     * These have 2 valid values:
     *      Unset:  0
     *      Set:    Must have equal number of 1's and 0's
     *
     * NB. The UniPro Mfgr's ID and PID are hard-wired into their DME
     * attributes, so there is no need to fetch/store them in this function.
     * However, ARA VID and PID are required by load_tftf_header(), and while
     * hardwired on real hardware, are not on the HAPS-62 or simulator. Rather
     * than write to read-only registers, so they can be retrieved in
     * load_tftf_header(), we validate them and cache them in the communication
     * area at the top of memory
     *
     * TA-13 Write/Read  DME attribute (New area of 16 words)
     * TA-03 Set e-Fuse data as SN, PID, VID, CMS, SCR, IMS and read...
     */
    ara_vid = tsb_get_ara_vid();
    if (!valid_hamming_weight((uint8_t *)&ara_vid,
                              sizeof(ara_vid),
                              &all_zero)) {
        dbgprintx32("Bad Ara VID: ", ara_vid, "\n");
        set_last_error(BRE_EFUSE_BAD_ARA_VID);
        return -1;
    }

    ara_pid = tsb_get_ara_pid();
    if (!valid_hamming_weight((uint8_t *)&ara_pid,
                              sizeof(ara_pid),
                              &all_zero)) {
        dbgprintx32("Bad Ara PID: ", ara_pid, "\n");
        set_last_error(BRE_EFUSE_BAD_ARA_PID);
        return -1;
    }

    /* Extract Internal Master Secret (IMS) from e­-Fuse, and if it is
     * non-zero, compute the Endpoint Unique ID
     */
    tsb_get_ims(ims_value, IMS_MEANINGFUL_LENGTH);

    if (!valid_hamming_weight((uint8_t *)ims_value,
                              IMS_MEANINGFUL_LENGTH,
                              &all_zero)) {
        dbgprint("Bad IMS\n");
        set_last_error(BRE_EFUSE_BAD_IMS);
        return -1;
    }

    if (!all_zero) {
#ifdef _NOCRYPTO
        /* Some fake value for simulation build */
        endpoint_id->low = 0x12345678;
        endpoint_id->high = 0x9ABCDEF0;
#else
        /* Calculate a real value from the IMS */
        calculate_es3_epuid(ims_value, &endpoint_id);
#endif
        dbgprintx64("Endpoint ID: ", endpoint_id.quad, "\n");
        urc = chip_unipro_attr_write(DME_ARA_ENDPOINTID_L, endpoint_id.low, 0,
                                ATTR_LOCAL);
        if (urc) {
            set_last_error(BRE_EFUSE_ENDPOINT_ID_WRITE);
            return -1;
        }
        urc = chip_unipro_attr_write(DME_ARA_ENDPOINTID_H, endpoint_id.high,
                                0, ATTR_LOCAL);
        if (urc) {
            set_last_error(BRE_EFUSE_ENDPOINT_ID_WRITE);
            return -1;
        }
    }

    dbgprint("efuse OK\n");
    return 0;
}

/**
 * JTAG is not at any time during Boot ROM operation, but has not been
 * permanently disabled. The Stage 2 firmware can choose to leave JTAG
 * disabled, enable JTAG, or permanently disable JTAG (until next RESET).
 *
 * On the other hand, IMS/CMS access should be permanently disabled (until
 * reset) for untrusted images
 **/
void efuse_rig_for_untrusted(void) {
    /* TA-21 Lock function with register (IMS, CMS) */
    tsb_disable_ims_access();
    tsb_disable_cms_access();
}


/**
 * @brief Count the number of "1" bits in an n-byte buffer
 *
 * @param buf The buffer to test
 * @param len The length in bytes of buf
 *
 * @returns The number of set bits in x (0 <= n <= 32)
 */
static int count_ones(uint8_t *buf, int len)
{
    uint8_t     x;
    uint32_t    x32;
    uint32_t *  buf32 = (uint32_t *) buf;
    int         count = 0;

    /* Process in 4-byte chunks as far as possible */
    while (len >= 4) {
        x32 = *buf32++;

        /* put count of each 2 bits into those 2 bits */
        x32 = (x32 & MASK32_1S) + ((x32 >> 1) & MASK32_1S);

        /* put count of each 4 bits into those 4 bits */
        x32 = (x32 & MASK32_2S) + ((x32 >> 2) & MASK32_2S);

        /* put count of each 8 bits into those 8 bits */
        x32 = (x32 & MASK32_4S) + ((x32 >> 4) & MASK32_4S);

        /* put count of each 16 bits into those 16 bits */
        x32 = (x32 & MASK32_8S) + ((x32 >> 8) & MASK32_8S);

        /* put count of each 16 bits into those 16 bits */
        x32 = (x32 + (x32 >> 16)) & MASK32_16S;

        count += x32;
        len -= 4;
    }

    /* Process any remaining bytes one at a time */
    buf = (uint8_t *)buf32;
    while (len > 0) {
        x = *buf++;

        /* put count of each 2 bits into those 2 bits */
        x -= (x >> 1) & MASK8_1S;

        /* put count of each 4 bits into those 4 bits */
        x = (x & MASK8_2S) + ((x >> 2) & MASK8_2S);

        /* put count of each 8 bits into those 8 bits */
        x = (x + (x >> 4)) & MASK8_4S;

        count += x;
        len--;
    }

    return count;
}


/**
 * @brief Verify that the buffer has the proper Hamming weight
 *
 * As specified in the high-level design, we validate certain e-Fuse
 * fields by their Hamming weight, for which there are 2 valid values:
 *      Unset:  0
 *      Set:    Equal numbers of 1's and 0's
 *
 * @param buf The buffer to test
 * @param len The length in bytes of buf
 * @param all_zero if all the bits in the buf are zero
 *
 * @returns The number of set bits in x (0 <= n <= 32)
 */
static bool valid_hamming_weight(uint8_t *buf, int len, bool *all_zero)
{
    int         count;

    count = count_ones(buf, len);

    *all_zero = (count == 0);

    return ((count == 0) ||
            (count == (len * 8 / 2)));
}

