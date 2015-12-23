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
 * @file mcl_rsa_runtime.h
 * @author Mike Scott and Kealan McCusker
 * @date 2nd June 2015
 * @brief RSA Header file for implementation of RSA protocol
 *
 * declares functions
 * 
 */

#ifndef MCL_RSA_RUNTIME_H
#define MCL_RSA_RUNTIME_H

#include "mcl_arch.h"
#include "mcl_config.h"
#include "mcl_rand.h"
#include "mcl_oct.h"
#include "mcl_hash.h"
#include "mcl_big.h"
#include "mcl_ff.h"

/******   Curve 1  *****/
#if MCL_CHOICE1==MCL_NIST256    
#define MCL_MBITS1 256
#endif

#if MCL_CHOICE1==MCL_C25519  
#define MCL_MBITS1 255
#endif

#if MCL_CHOICE1==MCL_C41417  
#define MCL_MBITS1 414
#endif

#if MCL_CHOICE1==MCL_C448 
#define MCL_MBITS1 448
#endif

#if MCL_CHOICE1==MCL_NIST384    
#define MCL_MBITS1 384
#endif

#if MCL_CHOICE1==MCL_NIST521    
#define MCL_MBITS1 521
#endif

#if MCL_CHUNK==16
#define MCL_BASEBITS1 13
#endif

#if MCL_CHUNK == 32
#if MCL_MBITS1 <= 360
#define MCL_BASEBITS1 29
#else

#if MCL_CHOICE1 == MCL_C448
#define MCL_BASEBITS1 27
#else
#define MCL_BASEBITS1 28
#endif

#endif
#endif

#if MCL_CHUNK == 64
#if MCL_MBITS1<=256 || MCL_CHOICE1 == MCL_NIST384
#define MCL_BASEBITS1 56
#else

#if MCL_CHOICE1 == MCL_C448
#define MCL_BASEBITS1 58
#else
#define MCL_BASEBITS1 60
#endif

#endif
#endif

#define MCL_MODBYTES1 (1+(MCL_MBITS1-1)/8) 
#define MCL_BIGBITS1 (MCL_MODBYTES1*8)
#define MCL_NLEN1 (1+((MCL_MBITS1-1)/MCL_BASEBITS1))
#define MCL_FF_BITS1 (MCL_BIGBITS1*MCL_FFLEN1)

typedef struct
{
  sign32 e;  
  mcl_chunk n[MCL_FFLEN1][MCL_NLEN1]; 
} MCL_rsa_public_key_DRRSA1;

typedef struct
{
  mcl_chunk p[MCL_FFLEN1/2][MCL_NLEN1];
  mcl_chunk q[MCL_FFLEN1/2][MCL_NLEN1];
  mcl_chunk dp[MCL_FFLEN1/2][MCL_NLEN1];
  mcl_chunk dq[MCL_FFLEN1/2][MCL_NLEN1];
  mcl_chunk c[MCL_FFLEN1/2][MCL_NLEN1];
} MCL_rsa_private_key_DRRSA1;


#define MCL_RFS1 (MCL_MODBYTES1*MCL_FFLEN1)
#define MCL_HASH_TYPE_RSA MCL_SHA256

extern void MCL_RSA_CREATE_CSPRNG_DRRSA1(csprng *R,mcl_octet *S);
extern void MCL_RSA_KILL_CSPRNG_DRRSA1(csprng *R);
extern void MCL_RSA_KEY_PAIR_DRRSA1(csprng *R,sign32 e,MCL_rsa_private_key_DRRSA1* PRIV,MCL_rsa_public_key_DRRSA1* PUB);
extern int MCL_PKCS15_DRRSA1(int h,mcl_octet *M,mcl_octet *W);
extern int MCL_OAEP_ENCODE_DRRSA1(int h,mcl_octet *M,csprng *R,mcl_octet *P,mcl_octet *F); 
extern int  MCL_OAEP_DECODE_DRRSA1(int h,mcl_octet *P,mcl_octet *F);
extern void MCL_RSA_ENCRYPT_DRRSA1(MCL_rsa_public_key_DRRSA1* PUB,mcl_octet *F,mcl_octet *G); 
extern void MCL_RSA_DECRYPT_DRRSA1(MCL_rsa_private_key_DRRSA1* PRIV,mcl_octet *G,mcl_octet *F);  
extern void MCL_RSA_PRIVATE_KEY_KILL_DRRSA1(MCL_rsa_private_key_DRRSA1 *PRIV);


/******  Curve 3  *****/
#if MCL_CHOICE3==MCL_NIST256    
#define MCL_MBITS3 256
#endif

#if MCL_CHOICE3==MCL_C25519  
#define MCL_MBITS3 255
#endif

#if MCL_CHOICE3==MCL_C41417  
#define MCL_MBITS3 414
#endif

#if MCL_CHOICE3==MCL_C448 
#define MCL_MBITS3 448
#endif

#if MCL_CHOICE3==MCL_NIST384    
#define MCL_MBITS3 384
#endif

#if MCL_CHOICE3==MCL_NIST521    
#define MCL_MBITS3 521
#endif

#if MCL_CHUNK==16
#define MCL_BASEBITS3 13
#endif

#if MCL_CHUNK == 32
#if MCL_MBITS3 <= 360
#define MCL_BASEBITS3 29
#else

#if MCL_CHOICE3 == MCL_C448
#define MCL_BASEBITS3 27
#else
#define MCL_BASEBITS3 28
#endif

#endif
#endif

#if MCL_CHUNK == 64
#if MCL_MBITS3<=256 || MCL_CHOICE3 == MCL_NIST384
#define MCL_BASEBITS3 56
#else

#if MCL_CHOICE3 == MCL_C448
#define MCL_BASEBITS3 58
#else
#define MCL_BASEBITS3 60
#endif

#endif
#endif

#define MCL_MODBYTES3 (1+(MCL_MBITS3-1)/8) 
#define MCL_BIGBITS3 (MCL_MODBYTES3*8)
#define MCL_NLEN3 (1+((MCL_MBITS3-1)/MCL_BASEBITS3))
#define MCL_FF_BITS3 (MCL_BIGBITS3*MCL_FFLEN3)

typedef struct
{
  sign32 e;  
  mcl_chunk n[MCL_FFLEN3][MCL_NLEN3]; 
} MCL_rsa_public_key_DRRSA3;

typedef struct
{
  mcl_chunk p[MCL_FFLEN3/2][MCL_NLEN3];
  mcl_chunk q[MCL_FFLEN3/2][MCL_NLEN3];
  mcl_chunk dp[MCL_FFLEN3/2][MCL_NLEN3];
  mcl_chunk dq[MCL_FFLEN3/2][MCL_NLEN3];
  mcl_chunk c[MCL_FFLEN3/2][MCL_NLEN3];
} MCL_rsa_private_key_DRRSA3;


#define MCL_RFS3 (MCL_MODBYTES3*MCL_FFLEN3)

extern void MCL_RSA_CREATE_CSPRNG_DRRSA3(csprng *R,mcl_octet *S);
extern void MCL_RSA_KILL_CSPRNG_DRRSA3(csprng *R);
extern void MCL_RSA_KEY_PAIR_DRRSA3(csprng *R,sign32 e,MCL_rsa_private_key_DRRSA3* PRIV,MCL_rsa_public_key_DRRSA3* PUB);
extern int MCL_PKCS15_DRRSA3(int h,mcl_octet *M,mcl_octet *W);
extern int MCL_OAEP_ENCODE_DRRSA3(int h,mcl_octet *M,csprng *R,mcl_octet *P,mcl_octet *F); 
extern int  MCL_OAEP_DECODE_DRRSA3(int h,mcl_octet *P,mcl_octet *F);
extern void MCL_RSA_ENCRYPT_DRRSA3(MCL_rsa_public_key_DRRSA3* PUB,mcl_octet *F,mcl_octet *G); 
extern void MCL_RSA_DECRYPT_DRRSA3(MCL_rsa_private_key_DRRSA3* PRIV,mcl_octet *G,mcl_octet *F);  
extern void MCL_RSA_PRIVATE_KEY_KILL_DRRSA3(MCL_rsa_private_key_DRRSA3 *PRIV);


#endif
