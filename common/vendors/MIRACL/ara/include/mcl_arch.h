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

/* ARAcrypt Architecture header file mcl_arch.h */


/* NOTE: There is only one user configurable section in this header - see below */

#ifndef MCL_ARCH_H
#define MCL_ARCH_H

#ifdef MCL_BUILD_TEST

#ifdef MCL_BUILD_ARM
#include <wmstdio.h>
#include <wmsdk.h>
#include <wm_os.h>
#define printf wmprintf
#else
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#endif // MCL_BUILD_ARM

#else
#ifndef NULL
#define NULL ((void*)0)
#endif
#endif // MCL_BUILD_TEST

#ifdef MCL_BUILD_ARM
#define C99
#endif

/*** START OF USER CONFIGURABLE SECTION - set architecture ***/
// #define MCL_CHUNK 32		/**< size of mcl_chunk in bits = wordlength of computer = 16, 32 or 64. Note not all curve options are supported on 16-bit processors - see rom.c */
/*** END OF USER CONFIGURABLE SECTION ***/

/* Support for C99?  Note for GCC need to explicitly include -std=c99 in command line */

#if __STDC_VERSION__ >= 199901L
/* C99 code */
#define C99
#else
/* Not C99 code */
#endif

#ifndef C99  /* You are on your own! These are for Microsoft C */
#define sign32 __int32			/**< 32-bit signed integer */
#define sign8 signed char		/**< 8-bit signed integer */
#define unsign32 unsigned __int32 /**< 32-bit unsigned integer */
#define unsign64 unsigned long long  /**< 64-bit unsigned integer */
#else
#include <stdint.h>
#define sign8 int8_t			/**< 8-bit signed integer */
#define sign32 int32_t			/**< 32-bit signed integer */
#define unsign32 uint32_t		/**< 32-bit unsigned integer */
#define unsign64 uint64_t		/**< 64-bit unsigned integer */
#endif

#define uchar unsigned char  /**<  Unsigned char */

/* Don't mess with anything below this line unless you know what you are doing */
/* This next is probably OK, but may need changing for non-C99-standard environments */

#if MCL_CHUNK==16
#ifndef C99
#define mcl_chunk __int16		/**< C type corresponding to word length */
#define mcl_dchunk __int32		/**< Always define double length mcl_chunk type if available */
#else
#define mcl_chunk int16_t		/**< C type corresponding to word length */
#define mcl_dchunk int32_t		/**< Always define double length mcl_chunk type if available */
#endif
#endif

#if MCL_CHUNK == 32
#ifndef C99
#define mcl_chunk __int32		/**< C type corresponding to word length */
#define mcl_dchunk __int64		/**< Always define double length mcl_chunk type if available */
#else
#define mcl_chunk int32_t		/**< C type corresponding to word length */
#define mcl_dchunk int64_t		/**< Always define double length mcl_chunk type if available */
#endif
#endif

#if MCL_CHUNK == 64

#ifndef C99
#define mcl_chunk __int64		/**< C type corresponding to word length */						
							/**< Note - no 128-bit type available    */
#else
#define mcl_chunk int64_t		/**< C type corresponding to word length */		
#ifdef __GNUC__
#define mcl_dchunk __int128		/**< Always define double length mcl_chunk type if available - GCC supports 128 bit type  ??? */
#endif
#endif
#endif

#ifdef mcl_dchunk
#define MCL_COMBA      /**< Use MCL_COMBA method for faster BN muls, sqrs and reductions */
#endif

/* Elliptic Curve modulus types */

#define MCL_NOT_SPECIAL 0			/**< Modulus of no exploitable form */
#define MCL_PSEUDO_MERSENNE 1		/**< Pseudo-mersenne modulus of form $2^n-c$  */
#define MCL_MCL_MONTGOMERY_FRIENDLY 3	/**< Montgomery Friendly modulus of form $2^a(2^b-c)-1$  */
#define MCL_GENERALISED_MERSENNE 2 /**< Generalised-mersenne modulus of form $2^n-2^m-1$  */  /** MCL_C448 */

/* Curve types */

#define MCL_WEIERSTRASS 0			/**< Short Weierstrass form curve  */
#define MCL_EDWARDS 1				/**< Edwards or Twisted Edwards curve  */
#define MCL_MONTGOMERY 2			/**< Montgomery form curve  */

/* Elliptic curves are defined over prime fields */
/* Here are some popular EC prime fields for which I have prepared curves. Feel free to specify your own. */

#define MCL_NIST256 0 /**< For the NIST 256-bit standard curve		- MCL_WEIERSTRASS only */
#define MCL_C25519 1  /**< Bernstein's Modulus 2^255-19			- MCL_EDWARDS or MCL_MONTGOMERY only */
#define MCL_C41417 2  /**< Bernstein et al Curve41417 2^414-17  - MCL_EDWARDS only */
#define MCL_NIST384 3 /**< For the NIST 384-bit standard curve		- MCL_WEIERSTRASS only */
#define MCL_NIST521 4 /**< For the NIST 521-bit standard curve		- MCL_WEIERSTRASS only */
#define MCL_C448 5  /**< For Goldilocks curve - MCL_EDWARDS only */  /* MCL_C448 */

#define MCL_BIG mcl_chunk *
#define DMCL_BIG mcl_chunk *

#endif
