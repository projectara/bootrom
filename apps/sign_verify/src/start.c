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
 * Special, never-to-be-released s3fw that verifies that it runs
 */

#include <stdint.h>
#include <stdbool.h>
#include "chipapi.h"
#include "tsb_scm.h"
#include "tsb_isaa.h"
#include "efuse.h"
#include "unipro.h"
#include "chipdef.h"
#include "debug.h"
#include "data_loading.h"
#include "crypto.h"
#include "bootrom.h"
#include "utils.h"
#include "string.h"
#include "unipro.h"

union large_uint {
  struct {
    uint32_t low;
    uint32_t high;
  };
  uint64_t quad;
  uint8_t buffer[8];
};

void tsb_get_ims(uint8_t * buf, uint32_t size);
void tsb_get_cms(uint8_t * buf, uint32_t size);
uint32_t tsb_get_scr(void);
uint32_t tsb_get_ara_vid(void);
uint32_t tsb_get_ara_pid(void);
uint64_t tsb_get_serial_no(void);
uint32_t tsb_get_jtag_enable(void);
uint32_t tsb_get_jtag_disable(void);

void check_ims_cms_access(void);

#ifdef _STANDBY_TEST
int chip_enter_hibern8_client(void);
int chip_exit_hibern8_client(void);
void resume_sequence_in_workram(void);
int standby_sequence(void);

void resume_point(void) {
    /**
     * NOTE:
     * This experiment code runs resume_point on the stack of boot ROM.
     * Real code needs to save/restore its own stack pointer.
     */
    resume_sequence_in_workram();

    check_ims_cms_access();
    while(1);
}

int enter_standby(void) {
    communication_area *p = (communication_area *)&_communication_area;

    p->resume_data.jtag_disabled = 1;
    p->resume_data.resume_address = (uint32_t)resume_point;
    p->resume_data.resume_address_complement = ~(uint32_t)resume_point;

    return standby_sequence();
}
#endif /* _STANDBY_TEST */

/**
 * @brief C entry point
 *
 * @param none
 *
 * @returns Nothing. Should never return.
 */
void bootrom_main(void) {
    chip_init();
    crypto_init();

    dbginit();

    dbgprint("\nES3 ASIC validation image\n");
    dbgprintx32("ARA VID:       0x", tsb_get_ara_vid(), "\n");
    dbgprintx32("ARA PID:       0x", tsb_get_ara_pid(), "\n");
    dbgprintx64("SERIAL NUMBER: 0x", tsb_get_serial_no(), "\n");
    dbgprintx32("SCR:           0x", tsb_get_scr(), "\n");
    dbgprintx32("JTAG ENABLE:   0x", tsb_get_jtag_enable(), "\n");
    dbgprintx32("JTAG DISABLE:  0x", tsb_get_jtag_disable(), "\n");
    dbgprintx32("IMS DISABLE:   0x", tsb_get_disable_ims_access(), "\n");
    dbgprintx32("CMS DISABLE:   0x", tsb_get_disable_cms_access(), "\n");

    check_ims_cms_access();

#ifdef _STANDBY_TEST
    enter_standby();
#endif

    /* Our work is done */
    while(1);
}

/**
 * Some mistake in ES3 boot ROM makes the EPUID of ES3 different
 * than the spec says
 */
void calculate_es3_epuid(unsigned char *ims_value,
                         union large_uint * endpoint_id) {
    /* same code used in ES3 boot ROM to generate the EUID */
    int i;
    unsigned char EP_UID[SHA256_HASH_DIGEST_SIZE];
    unsigned char Y1[SHA256_HASH_DIGEST_SIZE];
    unsigned char Z0[SHA256_HASH_DIGEST_SIZE];
    uint32_t temp;
    uint32_t *pims = (uint32_t *)ims_value;

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

void calculate_epuid(unsigned char *ims_value,
                     union large_uint * endpoint_id) {
    /* same code used in ES3 boot ROM to generate the EUID */
    int i;
    unsigned char EP_UID[SHA256_HASH_DIGEST_SIZE];
    unsigned char Y1[SHA256_HASH_DIGEST_SIZE];
    unsigned char Z0[SHA256_HASH_DIGEST_SIZE];
    uint32_t temp;
    uint32_t *pims = (uint32_t *)ims_value;

    hash_start();
    /*** grab IMS 4bytes at a time and feed that to hash_update */
    for (i = 0; i < 4; i++) {
        temp = pims[i] ^ 0x3d3d3d3d;
        hash_update((unsigned char *)&temp, sizeof(temp));
    }
    hash_final(Y1);

    hash_start();
    hash_update(Y1, SHA256_HASH_DIGEST_SIZE);
    temp = 0x01010101;
    for (i = 0; i < 8; i++) {;
        hash_update((unsigned char *)&temp, sizeof(temp));
    }
    hash_final(Z0);

    hash_start();
    hash_update(Z0, SHA256_HASH_DIGEST_SIZE);
    hash_final(EP_UID);

    memcpy(endpoint_id, EP_UID, 8);
}

void display_epuid(bool calculate, unsigned char *ims) {
    int rc;
    union large_uint endpoint_id;
    union large_uint endpoint_id_calc;

    rc = chip_unipro_attr_read(DME_DDBL2_ENDPOINTID_L, &endpoint_id.low, 0,
                                ATTR_LOCAL);
    rc |= chip_unipro_attr_read(DME_DDBL2_ENDPOINTID_H, &endpoint_id.high,
                                 0, ATTR_LOCAL);
    if (rc) {
        dbgprint("Failed to read Enpoint UID from DME\n");
        return;
    }
    dbgprintx64("Endpoint ID from DME:         ", endpoint_id.quad, "\n");
    if (calculate) {
        calculate_es3_epuid(ims, &endpoint_id_calc);
        dbgprintx64("Endpoint ID calculated (ES3): ", endpoint_id_calc.quad, "\n");
        calculate_epuid(ims, &endpoint_id_calc);
        dbgprintx64("Endpoint ID calculated:       ", endpoint_id_calc.quad, "\n");
    }
}

/**
 * @brief Verify that
 *
 * @param none
 *
 * @returns Nothing.
 */
void check_ims_cms_access(void) {
    unsigned char ims[TSB_ISAA_NUM_IMS_BYTES];
    unsigned char cms[TSB_ISAA_NUM_CMS_BYTES];
    bool zero_ims;
    bool zero_cms;
    unsigned char ims_hash[SHA256_HASH_DIGEST_SIZE];
    unsigned char cms_hash[SHA256_HASH_DIGEST_SIZE];
    int i;
    /* Only show the hash when VID/PID are zero */
    bool show_hash = (tsb_get_ara_vid() == 0) && (tsb_get_ara_pid() == 0);

    /* Get, verify and clear the IMS and CMS values quickly */
    tsb_get_ims(ims, TSB_ISAA_NUM_IMS_BYTES);
    zero_ims = is_constant_fill(ims, TSB_ISAA_NUM_IMS_BYTES, 0);
    display_epuid(!zero_ims, ims);
    if (!zero_ims) {
        hash_start();
        hash_update(ims, sizeof(ims));
        hash_final(ims_hash);
    }
    /* Clear the buffer for security */
    memset(ims, 0, sizeof(ims));

    tsb_get_cms(cms, TSB_ISAA_NUM_CMS_BYTES);
    zero_cms = is_constant_fill(cms, TSB_ISAA_NUM_CMS_BYTES, 0);
    if (!zero_cms) {
        hash_start();
        hash_update(cms, sizeof(cms));
        hash_final(cms_hash);
    }
    /* Clear the buffer for security */
    memset(cms, 0, sizeof(cms));

    /* Now that they've been cleared, we can issue debug messages */
    dbgprint("IMS access ");
    dbgprint((tsb_get_disable_ims_access() == 0)? "en" : "dis");
    dbgprint("abled\n");
    dbgprint("IMS reads ");
    dbgprint(zero_ims? "" : "non-");
    dbgprint("zero\n");
    if (!zero_ims && show_hash) {
        dbgprint("IMS hash:");
        for (i = 0; i < SHA256_HASH_DIGEST_SIZE; i++) {
            if ((i & 0x0F) == 0) {
                dbgprint("\n    ");
            }
            dbgprinthex8(ims_hash[i]);
            dbgprint(" ");
        }
        dbgprint("\n");
    }

    dbgprint("CMS access ");
    dbgprint((tsb_get_disable_cms_access() == 0)? "en" : "dis");
    dbgprint("abled\n");
    dbgprint("CMS reads ");
    dbgprint(zero_cms? "" : "non-");
    dbgprint("zero\n");
    if (!zero_cms && show_hash) {
        dbgprint("CMS hash:");
        for (i = 0; i < SHA256_HASH_DIGEST_SIZE; i++) {
            if ((i & 0x0F) == 0) {
                dbgprint("\n    ");
            }
            dbgprinthex8(cms_hash[i]);
            dbgprint(" ");
        }
        dbgprint("\n");
    }
}
