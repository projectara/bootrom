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

/* ARAcrypt Configuration header file */


/**
 * @file mcl_config.h
 * @author Mike Scott and Kealan McCusker
 * @date 9th September 2015
 * @brief Main Header File
 *
 * Allows some user configuration
 * defines structures
 * declares functions
 * 
 */

/* NOTE: There is only one user configurable section in this header - see below */

#ifndef MCL_CONFIG_H
#define MCL_CONFIG_H

/*** START OF USER CONFIGURABLE SECTION - set architecture ***/

// #define MCL_CHOICE MCL_NIST256	/**< Current choice of Elliptic Curve */
/* For some moduli only one type of curve is supported. For others there is a choice of MCL_WEIERSTRASS, MCL_EDWARDS or MCL_MONTGOMERY curves. */
// #define MCL_CURVETYPE MCL_WEIERSTRASS	/**< Note that all curve types may not be supported - see above */
// #define MCL_FFLEN 8  /**< 2^n multiplier of MCL_BIGBITS to specify supported Finite Field size, e.g 2048=256*2^3 where MCL_BIGBITS=256 */

/*** END OF USER CONFIGURABLE SECTION ***/

/* MCL_CHOICE is used to determine important certain constants here, and curve constants can be found in appropriate rom file */

/* Note that just 5 important values defined by this header. Most are computed from MCL_CHOICE, but can be entered directly 

MCL_FFLEN - Size of RSA size in MCL_BIGS
MCL_MBITS - Number of bits in MCL_Modulus
MCL_MODTYPE - MCL_Modulus type
MCL_BASEBITS - Number of active bits per word.
MCL_BS  - size of MCL_BIGs

*/

#if MCL_CHOICE==MCL_NIST256    
#define MCL_MBITS 256	/**< Number of bits in MCL_Modulus */
#define MCL_MODTYPE MCL_NOT_SPECIAL /**< MCL_Modulus type */
#endif

#if MCL_CHOICE==MCL_C25519  
#define MCL_MBITS 255	/**< Number of bits in MCL_Modulus */
#define MCL_MODTYPE MCL_PSEUDO_MERSENNE /**< MCL_Modulus type */
#endif

#if MCL_CHOICE==MCL_C41417  
#define MCL_MBITS 414	/**< Number of bits in MCL_Modulus */
#define MCL_MODTYPE MCL_PSEUDO_MERSENNE /**< MCL_Modulus type */
#endif

#if MCL_CHOICE==MCL_C448   /** MCL_C448 */
#define MCL_MBITS 448	/**< Number of bits in MCL_Modulus */
#define MCL_MODTYPE MCL_GENERALISED_MERSENNE /**< MCL_Modulus type */
#endif

#if MCL_CHOICE==MCL_NIST384    
#define MCL_MBITS 384	/**< Number of bits in MCL_Modulus */
#define MCL_MODTYPE  MCL_NOT_SPECIAL /**< MCL_Modulus type */
#endif

#if MCL_CHOICE==MCL_NIST521    
#define MCL_MBITS 521	/**< Number of bits in MCL_Modulus */
#define MCL_MODTYPE  MCL_PSEUDO_MERSENNE /**< MCL_Modulus type */
#endif

/* Choose MCL_BASEBITS. Requires some thought! 
   Set MCL_DEBUG_NORM to check for overflows 
   Must also ensure that MCL_COMBA multiplication does not cause overflow,
   so 2^(2*(N-1)-2*BPL) > W+2
   where N is word length, BPL is bits per limb, and W is number of words
   
   So for example 2^(2*63-2*60) > 9+2 (64>11) */

/* Each "limb" of a big number occupies at most (n-3) bits of an n-bit computer word. The most significant word must have at least 4 extra unused bits */ 
/* For example for 256-bit elliptic curves:- */
/* For n=64, use 5 words, use 56 bits per limb, leaving 5*56-256 = 24 bits */
/* For n=32, use 9 words, use 29 bits per limb, leaving 9*29-256 = 5 bits */
/* For n=16, use 20 words, use 13 bits per limb, leaving 20*13-256 = 4 bits */
/* For 414 bit curve */
/* For n=64, use 7 words, use 60 bits per limb, leaving 7*60-414 = 6 bits */
/* For n=32, use 15 words, use 28 bits per limb, leaving 15*28-414 = 6 bits */
/* For 336-bit curve */
/* For n=32, use 12 words, use 29 bits per limb, leaving 12*29-336 = 12 bits */
/* For n=64, use 6 words, use 60 bits per limb, leaving 6*60-336 = 24 bits */
/* For 384-bit curve */
/* For n=32, use 14 words, use 28 bits per limb, leaving 14*28-384 = 8 bits */
/* For n=64, use 7 words, use 56 bits per limb, leaving 7*56-384 = 8 bits */
/* For 521-bit curve */
/* For n=32, use 19 words, use 28 bits per limb, leaving 19*28-521 = 11 bits */
/* For n=64, use 9 words, use 60 bits per limb, leaving 9*60-521 = 19 bits */
/* For 448-bit curve */ 
/* For n=32, use 17 words, use 27 bits per limb, leaving 17*27-448 = 11 bits */
/* For n=64, use 8 words, use 58 bits per limb, leaving 8*58-448 = 16 bits */



#if MCL_CHUNK==16
#define MCL_BASEBITS 13			/**< Numbers represented to base 2*MCL_BASEBITS */
#endif

#if MCL_CHUNK == 32
#if MCL_MBITS <= 360
#define MCL_BASEBITS 29			/**< Numbers represented to base 2*MCL_BASEBITS */
#else

#if MCL_CHOICE == MCL_C448 /** MCL_C448 */
#define MCL_BASEBITS 27
#else
#define MCL_BASEBITS 28			/**< Numbers represented to base 2*MCL_BASEBITS */
#endif

#endif
#endif

#if MCL_CHUNK == 64
#if MCL_MBITS<=256 || MCL_CHOICE == MCL_NIST384
#define MCL_BASEBITS 56			/**< Numbers represented to base 2*MCL_BASEBITS */
#else

#if MCL_CHOICE == MCL_C448  /** MCL_C448 */
#define MCL_BASEBITS 58
#else
#define MCL_BASEBITS 60			/**< Numbers represented to base 2*MCL_BASEBITS */
#endif

#endif
#endif

/* For debugging Only. */
//#define MCL_DEBUG_REDUCE 
//#define MCL_DEBUG_NORM
//#define MCL_GET_STATS

#ifdef MCL_DEBUG_NORM
#define MCL_BS ((1+((MCL_MBITS-1)/MCL_BASEBITS))+1)	/**< Number of words in MCL_BIG (plus one extra). */
#else
#define MCL_BS (1+((MCL_MBITS-1)/MCL_BASEBITS))	/**< Number of words in MCL_BIG. */
#endif

#endif
