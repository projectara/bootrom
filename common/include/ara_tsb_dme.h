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

#ifndef __COMMON_INCLUDE_ARA_DME_H
#define __COMMON_INCLUDE_ARA_DME_H

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
#define DME_DDBL2_ENDPOINTID_H      0x6102
#define DME_DDBL2_ENDPOINTID_L      0x6103

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

#endif /* __COMMON_INCLUDE_ARA_DME_H */


