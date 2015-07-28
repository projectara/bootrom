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
#include <stdint.h>
#include <errno.h>
#include "chip.h"
#include "unipro.h"
#include "tsb_unipro.h"
#include "chipapi.h"

/**
 * @brief advertise the boot status
 * @param boot_status
 * @param result_code destination for advertisement result
 * @return 0 on success, <0 on error
 */
int chip_advertise_boot_status(uint32_t boot_status, uint32_t *result_code) {
    return chip_unipro_attr_write(DME_DDBL2_INIT_STATUS, boot_status, 0,
                                  ATTR_LOCAL, result_code);
}

/**
 * @brief advertise the boot type
 * @param result_code destination for advertisement result
 * @return 0 on success, <0 on error
 */
int chip_advertise_boot_type(uint32_t *result_code) {
    return chip_unipro_attr_write(DME_DDBL2_INIT_TYPE, INIT_TYPE_TOSHIBA, 0,
                                  ATTR_LOCAL, result_code);
}

/**
 * @brief reset UniPro before signalling readiness to boot firmware to switch
 */
void chip_reset_before_ready(void) {
    tsb_reset_before_ready();
}
