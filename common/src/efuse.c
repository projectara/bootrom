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
#include "chip.h"
#include "chipapi.h"
#include "debug.h"
#include "tsb_scm.h"
#include "tsb_isaa.h"
#include "unipro.h"
#include "efuse.h"


/* Mask values used by "count_ones" */
#define MASK_1S     ((uint8_t)0x55) /* binary: 0101... */
#define MASK_2S     ((uint8_t)0x33) /* binary: 00110011.. */
#define MASK_4S     ((uint8_t)0x0f) /* binary:  4 zeros,  4 ones ... */


union large_uint {
  struct {
    uint32_t low;
    uint32_t high;
  };
  uint64_t quad;
};

static uint32_t         vid_value;
static uint32_t         pid_value;
static union large_uint serial_number;
static uint8_t          ims_value[TSB_ISAA_NUM_IMS_BYTES];

#define IMS_LENGTH  35

/* Prototypes */
static int count_ones(uint8_t *buf, int len);
static bool valid_hamming_weight(uint8_t *buf, int len);
static bool is_buf_const(uint8_t *buf, uint32_t size, uint8_t val);
static bool get_endpoint_id(union large_uint * endpoint_id);

int efuse_init(void) {
    uint32_t    register_val;
    uint32_t    dme_write_result;
    union large_uint endpoint_id;


    /* Check for e­-Fuse CRC error
     * See ARA_ESx_APBridge_RegisterMap_revxxx.pdf
     */
    register_val = tsb_get_eccerror();
    dbgprint("efuse_init: EccError = ");dbgprinthex32(register_val);dbgprint("\r\n");
    if ((register_val & TSB_ECCERROR_ECC_ERROR) != 0) {
        dbgprint("efuse_init Efuse ECC error\r\n");
        return EHOSTDOWN;
    }


    /* Verify VID/PID/SN (in e­-Fuse) have proper Hamming weight
     * These have 2 valid values:
     *      Unset:  0
     *      Set:    Must have equal number of 1's and 0's
     */
    vid_value = tsb_get_vid();
    /*****/dbgprint("efuse_init: VID = ");dbgprinthex32(vid_value);dbgprint("\r\n");
    if (!valid_hamming_weight((uint8_t *)&vid_value, sizeof(vid_value))) {
        dbgprint("efuse_init: Invalid VID\r\n");
        return EFAULT;
    }

    pid_value = tsb_get_pid();
    /*****/dbgprint("efuse_init: PID = ");dbgprinthex32(pid_value);dbgprint("\r\n");
    if (!valid_hamming_weight((uint8_t *)&pid_value, sizeof(pid_value))) {
        dbgprint("efuse_init: Invalid PID\r\n");
        return EFAULT;
    }

    serial_number.quad = tsb_get_serial_no();
    /*****/dbgprint("efuse_init: SN =  ");dbgprinthex64(serial_number.quad);dbgprint("\r\n");
    if (!valid_hamming_weight((uint8_t *)&serial_number,
                              sizeof(serial_number))) {
        dbgprint("efuse_init: Invalid serial number\r\n");
        return EFAULT;
    }

    /* Extract Internal Master Secret (IMS) from e­-Fuse, and if it is
     * non-zero, compute the Endpoint Unique ID
     */
    (void)get_endpoint_id(&endpoint_id);
    /*****/dbgprint("efuse_init: endpoint ID: ");dbgprinthex64(endpoint_id.quad);dbgprint("\r\n");


    /* Advertise various values via DME attribute registers */
    chip_unipro_attr_write(DME_DDBL2_VID, vid_value, 0,
                           ATTR_LOCAL, &dme_write_result);
    chip_unipro_attr_write(DME_DDBL2_PID, pid_value, 0,
                           ATTR_LOCAL, &dme_write_result);
    chip_unipro_attr_write(DME_DDBL2_SERIALNO_L, serial_number.low, 0,
                           ATTR_LOCAL, &dme_write_result);
    chip_unipro_attr_write(DME_DDBL2_SERIALNO_H, serial_number.high, 0,
                           ATTR_LOCAL, &dme_write_result);
    chip_unipro_attr_write(DME_DDBL2_ENDPOINTID_L, endpoint_id.low, 0,
                           ATTR_LOCAL, &dme_write_result);
    chip_unipro_attr_write(DME_DDBL2_ENDPOINTID_H, endpoint_id.high, 0,
                           ATTR_LOCAL, &dme_write_result);

    return 0;
}


void efuse_rig_for_untrusted(void) {
    tsb_jtag_disable();
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
    int         count = 0;

    while (len > 0) {
        x = *buf++;

        /* put count of each 2 bits into those 2 bits */
        x -= (x >> 1) & MASK_1S;

        /* put count of each 4 bits into those 4 bits */
        x = (x & MASK_2S) + ((x >> 2) & MASK_2S);

        /* put count of each 8 bits into those 8 bits */
        x = (x + (x >> 4)) & MASK_4S;

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
 *
 * @returns The number of set bits in x (0 <= n <= 32)
 */
static bool valid_hamming_weight(uint8_t *buf, int len)
{
    int         count;

    count = count_ones(buf, len);

    return ((count == 0) ||
            (count == (len * 8 / 2)));
}


/**
 * @brief Determine if a buffer is filled with a constant value
 *
 * @param buf The buffer to test.
 * @param size The size in bytes of buf,
 * @param val The constant value to check against.
 *
 * @returns Nothing
 */
static bool is_buf_const(uint8_t *buf, uint32_t size, uint8_t val) {
    while (size > 0) {
        if (*buf++ != val) {
            return false;
        }
        size--;
    }

    return true;
}


/**
 * @brief Extract Internal Master Secret (IMS) from e­-Fuse
 *
 * If the IMS is non-zero, compute the Endpoint Unique ID from it.
 *
 * @param endpoint_id Pointer to a 64-bit field, into which will go the
 * calculated endpoint ID
 * @returns True if it calculated an endpoint ID, false if there was no IMS
 * from which to calculate it, or if the IMS was deemed invalid.
 */
static bool get_endpoint_id(union large_uint * endpoint_id) {
    /* Establish the default (i.e., no endpoint ID) */
    bool have_endpoint_id = false;
    endpoint_id->quad = 0;

    /* Get the IMS and determine the course of action */
    tsb_get_ims(ims_value, sizeof(ims_value));
    /*****/dbgprint("efuse_init: IMS =\r\n");dbgprinthexbuf((uint8_t *)&ims_value[0], IMS_LENGTH);dbgprint("\r\n");
    if (!is_buf_const(ims_value, sizeof(ims_value), 0)) {
        /* Compute Endpoint Unique ID */

        /***** THIS IS TBD UNTIL WE GET THE IMS->EID ALGORITHM *****/
        /*****/dbgprint("WARNING: Endpoint ID unavailable\r\n");
        endpoint_id->quad = 0;   /***** TODO: compute Endpoint Unique ID from IMS *****/
        have_endpoint_id =  true;
    }

    return have_endpoint_id;
}
