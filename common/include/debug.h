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

#include "chipapi.h"

#ifdef _DEBUG
    #define dbginit() chip_dbginit()
    #define dbgputc(x) chip_dbgputc(x)
    void dbgprint(char *str);
    void dbgprintbool(uint8_t flag);
    void dbgprinthex8(uint8_t num);
    void dbgprinthex32(uint32_t num);
    void dbgprinthex64(uint64_t num);
    void dbgprinthexbuf(uint8_t * buf, int len);
    void dbgprintx32(char * s1, uint32_t num, char * s2);
    void dbgprintx64(char * s1, uint64_t num, char * s2);
    void dbgprintxbuf(char * s1, uint8_t * buf, int len, char * s2);
    #define dbgflush() chip_dbgflush()
#else
    #define dbginit()
    #define dbgputc(x)
    #define dbgprint(x)
    #define dbgprintbool(x)
    #define dbgprinthex8(x)
    #define dbgprinthex32(x)
    #define dbgprinthex64(x)
    #define dbgprinthexbuf(buf,len)
    #define dbgprintx32(s1,x,s2)
    #define dbgprintx64(s1,x,s2)
    #define dbgprintxbuf(s1,buf,len,s2)
    #define dbgflush()
#endif /* _DEBUG */

#endif /* __COMMON_INCLUDE_DEBUG_H */
