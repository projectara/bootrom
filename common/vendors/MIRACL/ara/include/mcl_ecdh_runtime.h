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
 * @file mcl_ecdh_runtime.h
 * @author Mike Scott and Kealan McCusker
 * @date 2nd June 2015
 * @brief Runtime Header file for implementation of standard EC protocols
 *
 * declares functions
 * 
 */

#ifndef MCL_ECDH_RUNTIME_H
#define MCL_ECDH_RUNTIME_H

#include "mcl_arch.h"
#include "mcl_config.h"
#include "mcl_rand.h"
#include "mcl_oct.h"
#include "mcl_hash.h"
#include "mcl_aes.h"
#include "mcl_big.h"
#include "mcl_ecp.h"
#include "mcl_fp.h"

#define MCL_EAS 16 /**< AES Key size in bytes - 16 for 128 bits, 32 for 256 bits */
#define MCL_HASH_TYPE_ECC MCL_SHA256 /**< Hash Length in bytes for ECC */
#define MCL_ECDH_OK                     0     /**< Function completed without error */
#define MCL_ECDH_INVALID_PUBLIC_KEY    -2	/**< Public Key is Invalid */
#define MCL_ECDH_ERROR                 -3	/**< ECDH Internal Error */
#define MCL_ECDH_INVALID               -4	/**< ECDH Internal Error */


/******   Curve 1  *****/
#if MCL_CHOICE1==MCL_NIST256    
#define MCL_MBITS1 256	/**< Number of bits in MCL_Modulus */
#endif

#if MCL_CHOICE1==MCL_C25519  
#define MCL_MBITS1 255	/**< Number of bits in MCL_Modulus */
#endif

#if MCL_CHOICE1==MCL_C41417  
#define MCL_MBITS1 414	/**< Number of bits in MCL_Modulus */
#endif

#if MCL_CHOICE1==MCL_C448   /** MCL_C448 */
#define MCL_MBITS1 448	/**< Number of bits in MCL_Modulus */
#endif

#if MCL_CHOICE1==MCL_NIST384    
#define MCL_MBITS1 384	/**< Number of bits in MCL_Modulus */
#endif

#if MCL_CHOICE1==MCL_NIST521    
#define MCL_MBITS1 521	/**< Number of bits in MCL_Modulus */
#endif
#define MCL_EGS1 (1+(MCL_MBITS1-1)/8)  /**< Curve 1 ECC Group Size in bytes */
#define MCL_EFS1 (1+(MCL_MBITS1-1)/8)  /**< Curve 1 ECC Field Size in bytes */

extern void MCL_CREATE_CSPRNG_DREC1(csprng *R,mcl_octet *S);
extern void MCL_KILL_CSPRNG_DREC1(csprng *R);
extern void MCL_HASH_DREC1(int h,mcl_octet *I,mcl_octet *O);
extern int MCL_HMAC_DREC1(int h,mcl_octet *M,mcl_octet *K,int len,mcl_octet *tag);
extern void MCL_KDF2_DREC1(int h,mcl_octet *Z,mcl_octet *P,int len,mcl_octet *K);
extern void MCL_PBKDF2_DREC1(int h,mcl_octet *P,mcl_octet *S,int rep,int len,mcl_octet *K);
extern void MCL_AES_CBC_IV0_ENCRYPT_DREC1(mcl_octet *K,mcl_octet *P,mcl_octet *C);
extern int MCL_AES_CBC_IV0_DECRYPT_DREC1(mcl_octet *K,mcl_octet *C,mcl_octet *P);
extern int  MCL_ECP_KEY_PAIR_GENERATE_DREC1(csprng *R,mcl_octet *s,mcl_octet *W);
extern int  MCL_ECP_PUBLIC_KEY_VALIDATE_DREC1(int f,mcl_octet *W);
extern int MCL_ECPSVDP_DH_DREC1(mcl_octet *s,mcl_octet *W,mcl_octet *K);
extern void MCL_ECP_ECIES_ENCRYPT_DREC1(int h,mcl_octet *P1,mcl_octet *P2,csprng *R,mcl_octet *W,mcl_octet *M,int len,mcl_octet *V,mcl_octet *C,mcl_octet *T);
extern int MCL_ECP_ECIES_DECRYPT_DREC1(int h,mcl_octet *P1,mcl_octet *P2,mcl_octet *V,mcl_octet *C,mcl_octet *T,mcl_octet *U,mcl_octet *M);
extern int MCL_ECPSP_DSA_DREC1(int h,csprng *R,mcl_octet *s,mcl_octet *M,mcl_octet *c,mcl_octet *d);
extern int MCL_ECPVP_DSA_DREC1(int h,mcl_octet *W,mcl_octet *M,mcl_octet *c,mcl_octet *d);


/******   Curve 2  *****/
#if MCL_CHOICE2==MCL_NIST256    
#define MCL_MBITS2 256	/**< Number of bits in MCL_Modulus */
#endif

#if MCL_CHOICE2==MCL_C25519  
#define MCL_MBITS2 255	/**< Number of bits in MCL_Modulus */
#endif

#if MCL_CHOICE2==MCL_C41417  
#define MCL_MBITS2 414	/**< Number of bits in MCL_Modulus */
#endif

#if MCL_CHOICE2==MCL_C448   /** MCL_C448 */
#define MCL_MBITS2 448	/**< Number of bits in MCL_Modulus */
#endif

#if MCL_CHOICE2==MCL_NIST384    
#define MCL_MBITS2 384	/**< Number of bits in MCL_Modulus */
#endif

#if MCL_CHOICE2==MCL_NIST521    
#define MCL_MBITS2 521	/**< Number of bits in MCL_Modulus */
#endif
#define MCL_EGS2 (1+(MCL_MBITS2-1)/8)  /**< Curve 2 ECC Group Size in bytes */
#define MCL_EFS2 (1+(MCL_MBITS2-1)/8)  /**< Curve 2 ECC Field Size in bytes */


extern void MCL_CREATE_CSPRNG_DREC2(csprng *R,mcl_octet *S);
extern void MCL_KILL_CSPRNG_DREC2(csprng *R);
extern void MCL_HASH_DREC2(int h,mcl_octet *I,mcl_octet *O);
extern int MCL_HMAC_DREC2(int h,mcl_octet *M,mcl_octet *K,int len,mcl_octet *tag);
extern void MCL_KDF2_DREC2(int h,mcl_octet *Z,mcl_octet *P,int len,mcl_octet *K);
extern void MCL_PBKDF2_DREC2(int h,mcl_octet *P,mcl_octet *S,int rep,int len,mcl_octet *K);
extern void MCL_AES_CBC_IV0_ENCRYPT_DREC2(mcl_octet *K,mcl_octet *P,mcl_octet *C);
extern int MCL_AES_CBC_IV0_DECRYPT_DREC2(mcl_octet *K,mcl_octet *C,mcl_octet *P);
extern int  MCL_ECP_KEY_PAIR_GENERATE_DREC2(csprng *R,mcl_octet *s,mcl_octet *W);
extern int  MCL_ECP_PUBLIC_KEY_VALIDATE_DREC2(int f,mcl_octet *W);
extern int MCL_ECPSVDP_DH_DREC2(mcl_octet *s,mcl_octet *W,mcl_octet *K);
extern void MCL_ECP_ECIES_ENCRYPT_DREC2(int h,mcl_octet *P1,mcl_octet *P2,csprng *R,mcl_octet *W,mcl_octet *M,int len,mcl_octet *V,mcl_octet *C,mcl_octet *T);
extern int MCL_ECP_ECIES_DECRYPT_DREC2(int h,mcl_octet *P1,mcl_octet *P2,mcl_octet *V,mcl_octet *C,mcl_octet *T,mcl_octet *U,mcl_octet *M);
extern int MCL_ECPSP_DSA_DREC2(int h,csprng *R,mcl_octet *s,mcl_octet *M,mcl_octet *c,mcl_octet *d);
extern int MCL_ECPVP_DSA_DREC2(int h,mcl_octet *W,mcl_octet *M,mcl_octet *c,mcl_octet *d);


#endif

