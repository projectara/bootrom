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

#ifndef __COMMON_INCLUDE_ERROR_H
#define __COMMON_INCLUDE_ERROR_H

//include <stdint.h>
//#include <stddef.h>
#include "debug.h"

#define BRE_OK                      ((uint32_t)0x000000)

/* These are used to divide the 24-bits of errno pushed to BOOT_STATUS */
#define BRE_L1_FW_MASK              0x000000ff
#define BRE_L2_FW_MASK              0x0000ff00
#define BRE_L3_FW_MASK              0x00ff0000
#define BRE_L1_FW_SHIFT             0
#define BRE_L2_FW_SHIFT             8
#define BRE_L3_FW_SHIFT             16

/*
 * Level 1 Firmware error codes
 * (Error groups go up by 0x00000020)
 */
#define BRE_GROUP_MASK              0x0000e0

#define BRE_EFUSE_BASE              (0x000010)
#define BRE_EFUSE_ECC               ((uint32_t)(BRE_EFUSE_BASE + 0))
#define BRE_EFUSE_BAD_ARA_VID       ((uint32_t)(BRE_EFUSE_BASE + 1))
#define BRE_EFUSE_BAD_ARA_PID       ((uint32_t)(BRE_EFUSE_BASE + 2))
#define BRE_EFUSE_BAD_IMS           ((uint32_t)(BRE_EFUSE_BASE + 3))
#define BRE_EFUSE_BAD_SERIAL_NO     ((uint32_t)(BRE_EFUSE_BASE + 4))
#define BRE_EFUSE_UNIPRO_VID_READ   ((uint32_t)(BRE_EFUSE_BASE + 5))
#define BRE_EFUSE_UNIPRO_PID_READ   ((uint32_t)(BRE_EFUSE_BASE + 6))
#define BRE_EFUSE_ENDPOINT_ID_WRITE ((uint32_t)(BRE_EFUSE_BASE + 7))

#define BRE_TFTF_BASE               ((uint32_t)0x000020)
#define BRE_TFTF_LOAD_HEADER        ((uint32_t)(BRE_TFTF_BASE + 0))
#define BRE_TFTF_HEADER_SIZE        ((uint32_t)(BRE_TFTF_BASE + 1))
#define BRE_TFTF_MEMORY_RANGE       ((uint32_t)(BRE_TFTF_BASE + 2))
#define BRE_TFTF_SENTINEL           ((uint32_t)(BRE_TFTF_BASE + 3))
#define BRE_TFTF_NO_TABLE_END       ((uint32_t)(BRE_TFTF_BASE + 4))
#define BRE_TFTF_NON_ZERO_PAD       ((uint32_t)(BRE_TFTF_BASE + 5))
#define BRE_TFTF_LOAD_SIGNATURE     ((uint32_t)(BRE_TFTF_BASE + 6))
#define BRE_TFTF_VIDPID_MISMATCH    ((uint32_t)(BRE_TFTF_BASE + 7))
#define BRE_TFTF_COMPRESSION_UNSUPPORTED    ((uint32_t)(BRE_TFTF_BASE + 8))
#define BRE_TFTF_COMPRESSION_BAD    ((uint32_t)(BRE_TFTF_BASE + 9))
#define BRE_TFTF_HASHED_SECTION_AFTER_UNHASHED    ((uint32_t)(BRE_TFTF_BASE + 10))
#define BRE_TFTF_HEADER_TYPE        ((uint32_t)(BRE_TFTF_BASE + 11))
#define BRE_TFTF_COLLISION          ((uint32_t)(BRE_TFTF_BASE + 12))
#define BRE_TFTF_START_NOT_IN_CODE  ((uint32_t)(BRE_TFTF_BASE + 13))
#define BRE_TFTF_IMAGE_CORRUPTED    ((uint32_t)(BRE_TFTF_BASE + 14))
#define BRE_TFTF_LOAD_DATA          ((uint32_t)(BRE_TFTF_BASE + 15))
#define BRE_TFTF_UNTRUSTED_NOT_ALLOWED    ((uint32_t)(BRE_TFTF_BASE + 16))

#define BRE_FFFF_BASE               ((uint32_t)0x000040)
#define BRE_FFFF_LOAD_HEADER        ((uint32_t)(BRE_FFFF_BASE + 0))
#define BRE_FFFF_HEADER_SIZE        ((uint32_t)(BRE_FFFF_BASE + 1))
#define BRE_FFFF_MEMORY_RANGE       ((uint32_t)(BRE_FFFF_BASE + 2))
#define BRE_FFFF_SENTINEL           ((uint32_t)(BRE_FFFF_BASE + 3))
#define BRE_FFFF_NO_TABLE_END       ((uint32_t)(BRE_FFFF_BASE + 4))
#define BRE_FFFF_NON_ZERO_PAD       ((uint32_t)(BRE_FFFF_BASE + 5))
#define BRE_FFFF_BLOCK_SIZE         ((uint32_t)(BRE_FFFF_BASE + 6))
#define BRE_FFFF_FLASH_CAPACITY     ((uint32_t)(BRE_FFFF_BASE + 7))
#define BRE_FFFF_IMAGE_LENGTH       ((uint32_t)(BRE_FFFF_BASE + 8))
#define BRE_FFFF_HEADER_NOT_FOUND   ((uint32_t)(BRE_FFFF_BASE + 9))
#define BRE_FFFF_NO_FIRMWARE        ((uint32_t)(BRE_FFFF_BASE + 10))
#define BRE_FFFF_ELT_RESERVED_MEMORY    ((uint32_t)(BRE_FFFF_BASE + 11))
#define BRE_FFFF_ELT_ALIGNMENT          ((uint32_t)(BRE_FFFF_BASE + 12))
#define BRE_FFFF_ELT_COLLISION          ((uint32_t)(BRE_FFFF_BASE + 13))
#define BRE_FFFF_ELT_DUPLICATE          ((uint32_t)(BRE_FFFF_BASE + 14))
#define BRE_FFFF_LOGIC_ERROR            ((uint32_t)(BRE_FFFF_BASE + 15))

#define BRE_CRYPTO_BASE             ((uint32_t)0x000060)


/*
 * Level 2 Firmware error codes
 * (Note that these will be automatically shifted into the correct bit-field
 * by "set_last_error". Do not pre-shift them in your definitions!)
 */
/* TBD */


/*
 * Level 3 Firmware error codes
 * (Note that these will be automatically shifted into the correct bit-field
 * by "set_last_error". Do not pre-shift them in your definitions!)
 */
/* TBD */


/* Syntactic sugar */
#define reset_last_error()  init_last_error()


/* TODO: Add crypto error codes */
//#define BRE_CRYPTO_BASE             ((uint32_t)0x000060)

/* TODO: Add Load-over-Unipro error codes */
//#define BRE_UNIPRO_BASE             ((uint32_t)0x000080)


/**
 * @brief Bootloader-specific "set-errno" wrapper initializer
 */
void init_last_error(void);


/**
 * @brief Wrapper to set the bootloader-specific "errno" value
 *
 * Note: The first error is sticky (subsequent settings are ignored)
 *
 * @param errno A BRE_xxx error code to save
 */
void set_last_error(uint32_t err);


/**
 * @brief Wrapper to get the last bootloader-specific "errno" value
 *
 * @returns The last BRE_xxx error code saved.
 */
uint32_t get_last_error(void);


/**
 * @brief Merge the bootrom "errno" and the boot status
 *
 *
 * @param boot_status The boot_status to push out to the DME variable
 * @return The merged boot_status variable
 */
uint32_t merge_errno_with_boot_status(uint32_t boot_status);


/**
 * @brief Wrapper to set the boot status and stop
 *
 * This is a terminal execution node. All passengers must disembark
 *
 * @param errno A BRE_xxx error code to save
 */
void halt_and_catch_fire(uint32_t boot_status);

#endif /* __COMMON_INCLUDE_ERROR_H */
