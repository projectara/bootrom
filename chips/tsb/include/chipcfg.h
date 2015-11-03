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

/**
 * This file defines some chip specific macros that would used by common code
 */

#ifndef __ARCH_ARM_TSB_CHIPCFG_H
#define __ARCH_ARM_TSB_CHIPCFG_H
/**
 * the code in tsb_utils.S implemented the chip_delay for 200ns
 * The +1 here makes sure we delay no shorter than expected
 */
#define CHIP_NS_TO_DELAY(n) ((n / 200) + 1)

/**
 * This macro is used by tftf.c. This allow different chip to convert
 * the specified code/data address in TFTF header to chip specific address.
 * For example, on TSB chip this would be a direct type cast, but for
 * code running on AP or PC to verify the TFTF image, this could be converted
 * to an address in a buffer allocated for containing the code/data.
 */
#define CHIP_IMAGE_LOADING_DEST(addr) ((unsigned char *)addr)

#define MAX_TFTF_HEADER_SIZE_SUPPORTED 4096
#define MAX_FFFF_HEADER_SIZE_SUPPORTED 4096

#endif /* __ARCH_ARM_TSB_CHIPCFG_H */
