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

#ifndef __COMMON_INCLUDE_DEBUG_H
#define __COMMON_INCLUDE_DEBUG_H

/* Ensure that debug messages are suppressed in Production builds */
#ifdef _PRODUCTION
#undef _DEBUGMSGS
#endif

#include "chipapi.h"

#ifdef _DEBUGMSGS
    void dbginit(void);
    void dbgputc(int x);
    void dbgprint(char *str);
    void dbgprinthex8(uint8_t num);
    void dbgprinthex16(uint16_t num);
    void dbgprinthex32(uint32_t num);
    void dbgprinthex64(uint64_t num);
    void dbgprintx16(char * s1, uint16_t num, char * s2);
    void dbgprintx32(char * s1, uint32_t num, char * s2);
    void dbgprintx64(char * s1, uint64_t num, char * s2);
    #define dbgflush() chip_dbgflush()
#else
    #define dbginit()
    #define dbgputc(x)
    #define dbgprint(str)
    #define dbgprinthex8(num)
    #define dbgprinthex16(num)
    #define dbgprinthex32(num)
    #define dbgprinthex64(num)
    #define dbgprintx16(s1,num,s2)
    #define dbgprintx32(s1,num,s2)
    #define dbgprintx64(cs1,num,s2)
    #define dbgflush()
#endif /* _DEBUGMSGS */

#endif /* __COMMON_INCLUDE_DEBUG_H */
