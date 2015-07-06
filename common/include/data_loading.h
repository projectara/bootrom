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

#ifndef __COMMON_INCLUDE_DATA_LOADING_H
#define __COMMON_INCLUDE_DATA_LOADING_H

#include <stdint.h>
#include <stdbool.h>

typedef int (*data_loading_init)(void);

/**
 * "read" function for random access.
 * "load" function for serialized access.
 *
 * These functions are expected to read exactly the requested number of bytes.
 * So unlike tradition "read" function that returns bytes read, only 0 and -1
 * are valid return value here.
 *
 * For image loading methods supports only serialized access, such as UniPro,
 * "read" should be set to NULL.
 *
 * For image loading methods supports random access, such as SPI flash,
 * "load" should load data from the end of previous "load" or "read".
 * For the first "load" before any "read", the address starts at 0
 *
 * A call to "read" with 0 length can be used to set the address for the
 * following "load"
 *
 * The "hash" parameter indicates if the "load" function should call
 * "hash_update" to calculate the hash of data beling loaded.
 */
typedef int (*data_loading_read)(void *dest, uint32_t addr, uint32_t length);
typedef int (*data_loading_load)(void *dest, uint32_t length, bool hash);

typedef void (*data_loading_finish)(void);

typedef struct {
    data_loading_init init;
    data_loading_read read;
    data_loading_load load;
    data_loading_finish finish;
} data_load_ops;

#endif /* __COMMON_INCLUDE_DATA_LOADING_H */
