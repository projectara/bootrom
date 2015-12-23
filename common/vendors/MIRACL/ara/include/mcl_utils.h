/*************************************************************************
                                                                         *
Copyright (c) 2015>, MIRACL Ltd                                          *
All rights reserved.                                                     *
                                                                         *
This file is derived from the MIRACL for Ara SDK.                        *
                                                                         *
The MIRACL for Ara SDK provides developers with an                       *
extensive and efficient set of cryptographic functions.                  *
For further information about its features and functionalities           *
please refer to https://www.miracl.com                                   *
                                                                         *
Redistribution and use in source and binary forms, with or without       *
modification, are permitted provided that the following conditions are   *
met:                                                                     *
                                                                         *
 1. Redistributions of source code must retain the above copyright       *
    notice, this list of conditions and the following disclaimer.        *
                                                                         *
 2. Redistributions in binary form must reproduce the above copyright    *
    notice, this list of conditions and the following disclaimer in the  *
    documentation and/or other materials provided with the distribution. *
                                                                         *
 3. Neither the name of the copyright holder nor the names of its        *
    contributors may be used to endorse or promote products derived      *
    from this software without specific prior written permission.        *
                                                                         *
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS  *
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED    *
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A          *
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT       *
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,   *
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED *
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR   *
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF   *
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING     *
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS       *
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.             *
                                                                         *
**************************************************************************/

/**
 * @file mcl_utils.h
 * @author Mike Scott and Kealan McCusker
 * @date 2nd June 2015
 * @brief Functions to time code and hex encode / decode strings
 *
 * declares functions
 * 
 */
 

#ifndef MCL_UTILS_H
#define MCL_UTILS_H
 
#ifdef MCL_BUILD_ARM
#include <wmstdio.h>
#include <wmsdk.h>
#include <wm_os.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#endif

#ifdef MCL_BUILD_ARM
#define printf wmprintf
#endif

/**	@brief Get start time 
 *
	@return Time in microseconds
 */
#ifdef MCL_BUILD_ARM
extern unsigned int MCL_start_time();
#else
extern double MCL_start_time();
#endif

/**	@brief Get end time and calculate total time. The max time between the 
        calls to start_tim and MCL_end_time is 4294 seconds for the ARM SoC.

        @t1     Start time in microseconds
	@return Total time in microseconds
 */
#ifdef MCL_BUILD_ARM
extern unsigned int MCL_end_time(unsigned int t1);
#else
extern double MCL_end_time(double t1);
#endif

/** @brief Decode hex value
 *
	@param src     Hex encoded string
	@param dst     Binary string
	@param src_len length Hex encoded string
	@return 
*/
extern void MCL_hex2bin(char *src, char *dst, int src_len);

/** @brief Encode binary string
 *
	@param src     Binary string
	@param dst     Hex encoded string
	@param src_len length binary string
	@return 
*/
extern void MCL_bin2hex(char *src, char *dst, int src_len);

/** @brief Encode hex value and print to screen 
 *
	@param input string
	@param input length string
	@return 
*/
extern void MCL_print_hex(char *input, int len);

/** @brief Check test value is same as expected value
 *
	@param want Expected value
	@param got  Calculated value
	@return 
*/
extern int MCL_test_value(char * want, char * got);

#endif

