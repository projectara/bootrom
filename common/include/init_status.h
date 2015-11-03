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

#ifndef __COMMON_INCLUDE_INIT_STATUS_H
#define __COMMON_INCLUDE_INIT_STATUS_H

#define INIT_TYPE_TOSHIBA                                    (0x01260001)

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

#endif /* __COMMON_INCLUDE_INIT_STATUS_H */


