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
#include "communication_area.h"
#include "secret_keys.h"

#define BOOTROM_SIZE    (16 * 1024)

//void tsb_get_ims(uint8_t * buf, uint32_t size);
//void tsb_get_cms(uint8_t * buf, uint32_t size);
uint32_t tsb_get_scr(void);
uint32_t tsb_get_ara_vid(void);
uint32_t tsb_get_ara_pid(void);
uint64_t tsb_get_serial_no(void);
uint32_t tsb_get_jtag_enable(void);
uint32_t tsb_get_jtag_disable(void);

void display_epuid_ims_cms_info(void);
void report_bootrom_hash(void);
void report_communication_area(void);

char * shared_function_names [NUMBER_OF_SHARED_FUNCTIONS] = {
    "SHA256_INIT",
    "SHA256_PROCESS",
    "SHA256_HASH",
    "RSA2048_VERIFY",
    "ENTER_STANDBY",
};



/**
 * @brief Print a buffer in hex.
 *
 * @param s1 (optional) prefix string
 * @param buf The buffer to print
 * @param len The length in bytes of the buffer
 * @param s2 (optional) suffix string
 *
 * @returns Nothing.
 */
void dbgprintxbuf(char * s1, uint8_t * buf, size_t len, char * s2) {
    size_t i;

    dbgprint(s1);
    for (i = 0; i < len; i++) {
        dbgprinthex8(buf[i]);
        dbgprint(" ");
    }
    dbgprint(s2);
}


/**
 * @brief Print a pointer
 *
 * @param s1 (optional) tsb_get_cmsprefix string
 * @param ptr The pointer to print
 * @param s2 (optional) suffix string
 *
 * @returns Nothing.
 */
void dbgprintxptr(char * s1, void *ptr, char * s2) {
    dbgprintx32(s1, (uint32_t)ptr, s2);
}


/**
 * @brief Print a string with optional prefix and suffix parts.
 *
 * @param s1 (optional) prefix string
 * @param str The string to print
 * @param s2 (optional) suffix string
 *
 * @returns Nothing.
 */
void dbgprintx(char * s1, char *str, char * s2) {
    dbgprint(s1);
    dbgprint(str);
    dbgprint(s2);
}

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

    display_epuid_ims_cms_info();
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
    uint32_t unipro_mid = 0;
    uint32_t unipro_pid = 0;
    uint32_t ara_init_type = 0;
    uint32_t ara_init_status = 0;

    chip_init();
    crypto_init();
    dbginit();

    dbgprint("\nES3 ASIC validation image\n");
    if (chip_unipro_attr_read(DME_DDBL1_MANUFACTURERID, &unipro_mid, 0,
                              ATTR_LOCAL) == 0) {
        dbgprintx32("Unipro MID:           0x", unipro_mid, "\n");
    } else {
        dbgprint("Unipro MID:           unavailable\n");
    }
    if (chip_unipro_attr_read(DME_DDBL1_PRODUCTID, &unipro_pid, 0,
                              ATTR_LOCAL) == 0) {
        dbgprintx32("Unipro PID:           0x", unipro_pid, "\n");
    } else {
        dbgprint("Unipro PID:           unavailable\n");
    }
    if (chip_unipro_attr_read(DME_ARA_INIT_TYPE, &ara_init_type, 0,
                              ATTR_LOCAL) == 0) {
        dbgprintx32("ARA Boot Protocol: 0x", ara_init_type, "\n");
    } else {
        dbgprint("ARA Boot Status:      unavailable\n");
    }
    if (chip_unipro_attr_read(DME_ARA_INIT_STATUS, &ara_init_status, 0,
                              ATTR_LOCAL) == 0) {
        dbgprintx32("ARA Boot Status:      0x", ara_init_status, "\n");
    } else {
        dbgprint("ARA Boot Status:      unavailable\n");
    }
    dbgprintx32("ARA VID:              0x", tsb_get_ara_vid(), "\n");
    dbgprintx32("ARA PID:              0x", tsb_get_ara_pid(), "\n");
    dbgprintx64("SERIAL NUMBER:        0x", tsb_get_serial_no(), "\n");
    dbgprintx32("SCR:                  0x", tsb_get_scr(), "\n");
    dbgprintx32("JTAG ENABLE:          0x", tsb_get_jtag_enable(), "\n");
    dbgprintx32("JTAG DISABLE:         0x", tsb_get_jtag_disable(), "\n");
    dbgprintx32("IMS DISABLE:          0x", tsb_get_disable_ims_access(),
                "\n");
    dbgprintx32("CMS DISABLE:          0x", tsb_get_disable_cms_access(),
                "\n");

    display_epuid_ims_cms_info();
    report_bootrom_hash();
    report_communication_area();

#ifdef _STANDBY_TEST
    enter_standby();
#endif

    /* Our work is done */
    while(1);
}

void display_epuid(bool calculate, unsigned char *ims) {
    int rc;
    union large_uint endpoint_id;
    union large_uint endpoint_id_calc;

    rc = chip_unipro_attr_read(DME_ARA_ENDPOINTID_L, &endpoint_id.low, 0,
                                ATTR_LOCAL);
    rc |= chip_unipro_attr_read(DME_ARA_ENDPOINTID_H, &endpoint_id.high,
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
 * @brief Report on IMS and CMS value accessibility.
 *
 * @param none
 *
 * @returns Nothing.
 */
void display_epuid_ims_cms_info(void) {
    unsigned char ims[TSB_ISAA_NUM_IMS_BYTES];
    unsigned char cms[TSB_ISAA_NUM_CMS_BYTES];
    bool zero_ims;
    bool zero_cms;
#ifdef IMS_CMS_HASH
    unsigned char ims_hash[SHA256_HASH_DIGEST_SIZE];
    unsigned char cms_hash[SHA256_HASH_DIGEST_SIZE];
#endif

    /* Get, verify and clear the IMS and CMS values quickly */
    tsb_get_ims(ims, TSB_ISAA_NUM_IMS_BYTES);
    zero_ims = is_constant_fill(ims, TSB_ISAA_NUM_IMS_BYTES, 0);
    display_epuid(true, ims);
#ifdef IMS_CMS_HASH
    hash_start();
    hash_update(ims, sizeof(ims));
    hash_final(ims_hash);
#endif
    /* Clear the buffer for security */
    memset(ims, 0, sizeof(ims));


    tsb_get_cms(cms, TSB_ISAA_NUM_CMS_BYTES);
    zero_cms = is_constant_fill(cms, TSB_ISAA_NUM_CMS_BYTES, 0);
#ifdef IMS_CMS_HASH
    hash_start();
    hash_update(cms, sizeof(cms));
    hash_final(cms_hash);
#endif
    /* Clear the buffer for security */
    memset(cms, 0, sizeof(cms));

    /* Now that they've been cleared, we can issue debug messages */
    dbgprint("IMS access ");
    dbgprint((tsb_get_disable_ims_access() == 0)? "en" : "dis");
    dbgprint("abled\n");
    dbgprint("IMS reads ");
    if (!zero_ims) {
        dbgprint("non-");
    }
    dbgprint("zero\n");
#ifdef IMS_CMS_HASH
    dbgprintxbuf("IMS hash: ", ims_hash, sizeof(ims_hash), "\n");
#endif /* IMS_CMS_HASH */

    dbgprint("CMS access ");
    dbgprint((tsb_get_disable_cms_access() == 0)? "en" : "dis");
    dbgprint("abled\n");
    dbgprint("CMS reads ");
    if (!zero_cms) {
        dbgprint("non-");
    }
    dbgprint("zero\n");
#ifdef IMS_CMS_HASH
    dbgprintxbuf("CMS hash: ", cms_hash, sizeof(cms_hash), "\n");
#endif /* IMS_CMS_HASH */
}

/**
 * @brief Report the BootRom SHA.
 *
 * @param none
 *
 * @returns Nothing.
 */
void report_bootrom_hash(void) {
    uint8_t bootrom_hash[SHA256_HASH_DIGEST_SIZE];

    /* Calculate the SHA256 for the bootrom... */
    hash_start();
    hash_update((unsigned char *)0x00000000, BOOTROM_SIZE);
    hash_final(bootrom_hash);

    /* ...and display it */
    dbgprintxbuf("bootrom hash: ", bootrom_hash, sizeof(bootrom_hash), "\n");
}



/**
 * @brief Report all relevant data from Communication Area
 *
 * Report all relevant data from Communication Area, including:
 *   - File version (name)
 *   - Build time
 *   - Validation key name
 *   - Firmware Identifier
 *   - Function pointers
 *
 * @param none
 *
 * @returns Nothing.
 */
void report_communication_area(void) {
    communication_area *comm_area = (communication_area *)&_communication_area;
    char text_buf[512];
    int i;

    dbgprint("Communication Area:\n");

    /* shared_functions */
    dbgprint("shared_functions:\n");
    for (i = 0; i < NUMBER_OF_SHARED_FUNCTIONS; i++) {
        dbgprintx("  ", shared_function_names[i], NULL);
        dbgprintxptr("  0x", comm_area->shared_functions[i], "\n");
    }
    dbgprint("\n");

    /* endpoint_unique_id */
    dbgprintxbuf("endpoint_unique_id: ",
            comm_area->endpoint_unique_id,
            sizeof(comm_area->endpoint_unique_id),
            "\n");

    /* stage_2_firmware_identity */
    dbgprintxbuf("stage_2_firmware_identity: ",
                 comm_area->stage_2_firmware_identity,
                 sizeof(comm_area->stage_2_firmware_identity),
                 "\n");

    /* stage_2_validation_key_name */
    memcpy(text_buf, comm_area->stage_2_validation_key_name,
           sizeof(comm_area->stage_2_validation_key_name));
    text_buf[sizeof(comm_area->stage_2_validation_key_name)] = '\0';
    dbgprintx("stage_2_validation_key_name: '", text_buf, "'\n");

    /* build_timestamp */
    memcpy(text_buf, comm_area->build_timestamp,
           sizeof(comm_area->build_timestamp));
    text_buf[sizeof(comm_area->build_timestamp)] = '\0';
    dbgprintx("build_timestamp: '", text_buf, "'\n");

    /* firmware_package_name */
    memcpy(text_buf, comm_area->firmware_package_name,
           sizeof(comm_area->firmware_package_name));
    text_buf[sizeof(comm_area->firmware_package_name)] = '\0';
    dbgprintx("firmware_package_name: '", text_buf, "'\n");
}

