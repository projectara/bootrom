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

#ifndef __COMMON_INCLUDE_ARA_UNIPRO_DME_H
#define __COMMON_INCLUDE_ARA_UNIPRO_DME_H

/**
 * This file defines ARA specific DME attributes
 */
#define DME_DDBL2_VID               0x6000
#define DME_DDBL2_PID               0x6001
#define DME_DDBL2_SERIALNO_L        0x6002
#define DME_DDBL2_SERIALNO_H        0x6003
#define DME_DDBL2_INIT_TYPE         0x6100
    #define INIT_TYPE_TOSHIBA                                    (0x01260001)
#define DME_DDBL2_INIT_STATUS       0x6101
    #define INIT_STATUS_UNINITIALIZED                            (0x000000000)
    #define INIT_STATUS_OPERATING                                (1 << 24)
    #define INIT_STATUS_SPI_BOOT_STARTED                         (2 << 24)
    #define INIT_STATUS_TRUSTED_SPI_FLASH_BOOT_FINISHED          (3 << 24)
    #define INIT_STATUS_UNTRUSTED_SPI_FLASH_BOOT_FINISHED        (4 << 24)
    #define INIT_STATUS_UNIPRO_BOOT_STARTED                      (6 << 24)
    #define INIT_STATUS_TRUSTED_UNIPRO_BOOT_FINISHED             (7 << 24)
    #define INIT_STATUS_UNTRUSTED_UNIPRO_BOOT_FINISHED           (8 << 24)
    #define INIT_STATUS_FALLLBACK_UNIPRO_BOOT_STARTED            (9 << 24)
    #define INIT_STATUS_FALLLBACK_TRUSTED_UNIPRO_BOOT_FINISHED   (10 << 24)
    #define INIT_STATUS_FALLLBACK_UNTRUSTED_UNIPRO_BOOT_FINISHED (11 << 24)
    #define INIT_STATUS_RESUMED_FROM_STANDBY                     (12 << 24)
    #define INIT_STATUS_FAILED                                   (0x80000000)
    #define INIT_STATUS_ERROR_MASK                               (0x80000000)
    #define INIT_STATUS_STATUS_MASK                              (0x7f000000)
    #define INIT_STATUS_ERROR_CODE_MASK                          (0x00ffffff)
#define DME_DDBL2_ENDPOINTID_H      0x6102
#define DME_DDBL2_ENDPOINTID_L      0x6103
#define DME_POWERMODEIND            0xd040
    #define TSB_DME_POWERMODEIND_NONE       (0) // no new value since last read
    #define TSB_DME_POWERMODEIND_OK         (1 << 0)
    #define TSB_DME_POWERMODEIND_LOCAL      (1 << 1)
    #define TSB_DME_POWERMODEIND_REMOTE     (1 << 2)
    #define TSB_DME_POWERMODEIND_BUSY       (1 << 3)
    #define TSB_DME_POWERMODEIND_CAP_ERR    (1 << 4)
    #define TSB_DME_POWERMODEIND_FATAL_ERR  (1 << 5)
#define DME_FC0PROTECTIONTIMEOUTVAL 0xd041
#define DME_TC0REPLAYTIMEOUTVAL     0xd042
#define DME_AFC0REQTIMEOUTVAL       0xd043
#define DME_FC1PROTECTIONTIMEOUTVAL 0xd044
#define DME_TC1REPLAYTIMEOUTVAL     0xd045
#define DME_AFC1REQTIMEOUTVAL       0xd046

/*
 * Switch attributes and related values
 */
#define ARA_INTERRUPTSTATUS        0xd081
    #define ARA_INTERRUPTSTATUS_MAILBOX (1 << 15)
#define ARA_MAILBOX                0xa000
    #define ARA_MAIL_RESET              (0x00)
    #define ARA_MAIL_READY_AP           (0x01)
    #define ARA_MAIL_READY_OTHER        (0x02)
#define ARA_MBOX_ACK_ATTR           0x610f

#endif /* __COMMON_INCLUDE_ARA_UNIPRO_DME_H */


