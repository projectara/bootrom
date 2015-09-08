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

#ifndef __COMMON_INCLUDE_BOOTROM_H
#define __COMMON_INCLUDE_BOOTROM_H

#include <stdint.h>

typedef struct {
    uint32_t resume_address;
    uint32_t resume_address_complement;
} __attribute__ ((packed)) resume_address_communication_area;

/*
 * Area of memory used to communicate between boot ROM and second stage FW.
 * This area is located at the highest end of the RAM. So any addition to the
 * area needs to happen BEFORE the existing fields.
 *
 * NOTE: COMMUNICATION_AREA_LENGTH must match _communication_area_size in
 * common.ld!
 */
#define COMMUNICATION_AREA_LENGTH   1024
#ifdef _SIMULATION
  #define DBGPRINT_BUF_LENGTH   128
#else
  #define DBGPRINT_BUF_LENGTH   0
#endif
#define ARA_VID_LENGTH      (sizeof(uint32_t))
#define ARA_POID_LENGTH      (sizeof(uint32_t))
#define EUID_LENGTH         8
#define S2_FW_ID_LENGTH     32
#define S2_KEY_NAMELENGTH   96
#define S2_FW_DESC_LENGTH   64
#define RESUME_ADDR_LENGTH  (sizeof(resume_address_communication_area))
#define PAD_LENGTH  (COMMUNICATION_AREA_LENGTH - \
                    (ARA_VID_LENGTH + ARA_PID_LENGTH + \
                     DBGPRINT_BUF_LENGTH + EUID_LENGTH + S2_FW_ID_LENGTH + \
                     S2_KEY_NAMELENGTH + S2_FW_DESC_LENGTH + \
                     RESUME_ADDR_LENGTH))

typedef struct {
#ifdef _SIMULATION
    char dbgprint_buf[DBGPRINT_BUF_LENGTH];
#endif
    unsigned char padding[PAD_LENGTH];
    /***** ADD NEW VARIABLES BELOW THIS LINE AND UPDATE "PAD_LENGTH" *****/
    uint32_t ara_vid;
    uint32_t ara_pid;
    unsigned char endpoint_unique_id[EUID_LENGTH];
    unsigned char stage_2_firmware_identity[S2_FW_ID_LENGTH];
    char stage_2_validation_key_name[S2_KEY_NAMELENGTH];
    char stage_2_firmware_description[S2_FW_DESC_LENGTH];
    resume_address_communication_area resume_address;
} __attribute__ ((packed)) communication_area;

extern unsigned char _communication_area;

#endif /* __COMMON_INCLUDE_BOOTROM_H */
