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

#ifndef __ARCH_ARM_SRC_TSB_TSB_UNIPRO_DME_H
#define __ARCH_ARM_SRC_TSB_TSB_UNIPRO_DME_H

/**
 * This file defines TSB specific DME attributes
 */

#define TSB_HIBERNATE_ENTER_REQ    0xd030
#define TSB_HIBERNATE_ENTER_IND    0xd031
#define TSB_HIBERNATE_EXIT_REQ     0xd032
#define TSB_HIBERNATE_EXIT_IND     0xd033

/* TSB_PowerState */
#define TSB_POWERSTATE             0xd083
    #define POWERSTATE_DISABLED     (0x00)
    #define POWERSTATE_LINKDOWN     (0x01)
    #define POWERSTATE_LINKUP       (0x02)
    #define POWERSTATE_HIBERNATE    (0x03)
    #define POWERSTATE_LINKLOST     (0x04)
    #define POWERSTATE_LINKCFG      (0x05)

#define TSB_MAXSEGMENTCONFIG        0xd089

#endif /* __ARCH_ARM_SRC_TSB_TSB_UNIPRO_DME_H */

