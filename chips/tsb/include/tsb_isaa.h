/*
 * Copyright (c) 2014 Google Inc.
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
 * @brief: This file describes the ISAA register access routines.
 *
 * (ISAA = Information Security And Assurance register)
 *
 * External documents:
 *      "ARA_ES3_APBridge_AppendixD_rev010.pdf
 *      "ARA_ES3_APBridge_RegisterMap_AppendixD_rev001.pdf
 *
 * Author: Morgan Girling <morgang@bsquare.com>
 */

#ifndef __ARCH_ARM_SRC_TSB_TSB_ISAA_H
#define __ARCH_ARM_SRC_TSB_TSB_ISAA_H

#include "chip.h"

#define BITS_TO_BYTES(bits) \
    (((bits) + 7) >> 3)

/* Size the IMS and CMS fields */
#define TSB_ISAA_NUM_IMS_BITS   280
#define TSB_ISAA_NUM_CMS_BITS   192
#define TSB_ISAA_NUM_IMS_BYTES  BITS_TO_BYTES(TSB_ISAA_NUM_IMS_BITS)
#define TSB_ISAA_NUM_CMS_BYTES  BITS_TO_BYTES(TSB_ISAA_NUM_CMS_BITS)


/**
 * @brief Read the 64-bit serial number (SN) register
 *
 * @param None
 *
 * @returns Nothing
 */
uint64_t tsb_get_serial_no(void);


/**
 * @brief Read the 280 bit (35 byte) IMS value
 *
 * @param buf Points to the buffer to which the IMS will be copied.
 * @param size The size in bytes of buf.
 *
 * @returns Nothing
 */
void tsb_get_ims(uint8_t * buf, uint32_t size);

/**
 * @brief Read the CMS value
 *
 * @param buf Points to the buffer to which the CMS will be copied.
 * @param size The size in bytes of buf.
 *
 * @returns Nothing
 */
void tsb_get_cms(uint8_t * buf, uint32_t size);



/**
 * @brief Read the Disable IMS access register
 *
 * @param None.
 *
 * @returns The contents of the disable_ims_access register
 */
uint32_t tsb_get_disable_ims_access(void);



/**
 * @brief Disable IMS access (cleared on h/w reset)
 *
 * @param None.
 *
 * @returns Nothing
 */
void tsb_disable_ims_access(void);


/**
 * @brief Read the disable_cmd_access register
 *
 * @param None.
 *
 * @returns The contents of the disable_cmd_access register
 */
uint32_t tsb_get_disable_cms_access(void);


/**
 * @brief Disable CMS access (cleared on h/w reset)
 *
 * @param None.
 *
 * @returns Nothing
 */
void tsb_disable_cms_access(void);


/**
 * @brief Disable JTAG access (cleared on h/w reset)
 *
 * @param None.
 *
 * @returns Nothing
 */
void tsb_jtag_disable(void);

/* TODO: Remove after bootloader completion if not used */
uint32_t tsb_get_scr(void);

/* TODO: Remove after bootloader completion if not used */
uint32_t tsb_get_retest(void);

/* TODO: Remove after bootloader completion if not used */
uint32_t tsb_get_jtag_enable(void);

/* TODO: Remove after bootloader completion if not used */
/*****/uint32_t tsb_get_software_jtag_control(void);

#endif /* __ARCH_ARM_SRC_TSB_TSB_ISAA_H */
