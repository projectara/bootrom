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
 * @file mcl_rsa.h
 * @author Mike Scott and Kealan McCusker
 * @date 2nd June 2015
 * @brief RSA Header file for implementation of RSA protocol
 *
 * declares functions
 * 
 */

#ifndef MCL_RSA_H
#define MCL_RSA_H

#include "mcl_arch.h"
#include "mcl_config.h"
#include "mcl_rand.h"
#include "mcl_oct.h"
#include "mcl_hash.h"
#include "mcl_big.h"
#include "mcl_ff.h"

#define MCL_MODBYTES (1+(MCL_MBITS-1)/8) /**< Number of bytes in MCL_Modulus */
#define MCL_BIGBITS (MCL_MODBYTES*8) /**< Number of bits representable in a MCL_BIG */
#define MCL_NLEN (1+((MCL_MBITS-1)/MCL_BASEBITS))	/**< Number of words in MCL_BIG. */
#define MCL_FF_BITS (MCL_BIGBITS*MCL_FFLEN) /**< Finite Field Size in bits - must be MCL_BIGBITS.2^n */

/**
	@brief Integer Factorisation Public Key
*/

typedef struct
{
    sign32 e;     /**< RSA exponent (typically 65537) */
    mcl_chunk n[MCL_FFLEN][MCL_NLEN]; /**< An array of MCL_BIGs to store public key */
} MCL_rsa_public_key;

/**
	@brief Integer Factorisation Private Key
*/

typedef struct
{
    mcl_chunk p[MCL_FFLEN/2][MCL_NLEN]; /**< secret prime p  */
    mcl_chunk q[MCL_FFLEN/2][MCL_NLEN]; /**< secret prime q  */
    mcl_chunk dp[MCL_FFLEN/2][MCL_NLEN]; /**< decrypting exponent mod (p-1)  */
    mcl_chunk dq[MCL_FFLEN/2][MCL_NLEN]; /**< decrypting exponent mod (q-1)  */
    mcl_chunk c[MCL_FFLEN/2][MCL_NLEN];  /**< 1/p mod q */
} MCL_rsa_private_key;


#define MCL_RFS (MCL_MODBYTES*MCL_FFLEN) /**< RSA Public Key Size in bytes */
#define MCL_HASH_TYPE_RSA MCL_SHA256 /**< Chosen Hash algorithm */


/* RSA Auxiliary Functions */
/**	@brief Initialise a random number generator
 *
	@param R is a pointer to a cryptographically secure random number generator
	@param S is an input truly random seed value
 */
extern void MCL_RSA_CREATE_CSPRNG(csprng *R,mcl_octet *S);
/**	@brief Kill a random number generator
 *
	Deletes all internal state
	@param R is a pointer to a cryptographically secure random number generator
 */
extern void MCL_RSA_KILL_CSPRNG(csprng *R);
/**	@brief RSA Key Pair Generator
 *
	@param R is a pointer to a cryptographically secure random number generator
	@param e the encryption exponent
	@param PRIV the output RSA private key
	@param PUB the output RSA public key
 */
extern void MCL_RSA_KEY_PAIR(csprng *R,sign32 e,MCL_rsa_private_key* PRIV,MCL_rsa_public_key* PUB);
/**	@brief PKCS V1.5 padding of a message prior to RSA signature
 *
	@param h is the hash type
	@param M is the input message
	@param F is the output encoding, ready for RSA signature
	@return 1 if OK, else 0
 */
extern int MCL_PKCS15(int h,mcl_octet *M,mcl_octet *W);
/**	@brief OAEP padding of a message prior to RSA encryption
 *
	@param h is the hash type
	@param M is the input message
	@param R is a pointer to a cryptographically secure random number generator
	@param P are input encoding parameter string (could be NULL)
	@param F is the output encoding, ready for RSA encryption
	@return 1 if OK, else 0
 */
extern int	MCL_OAEP_ENCODE(int h,mcl_octet *M,csprng *R,mcl_octet *P,mcl_octet *F); 
/**	@brief OAEP unpadding of a message after RSA decryption
 *
	Unpadding is done in-place
	@param h is the hash type
	@param P are input encoding parameter string (could be NULL)
	@param F is input padded message, unpadded on output
	@return 1 if OK, else 0
 */
extern int  MCL_OAEP_DECODE(int h,mcl_octet *P,mcl_octet *F);
/**	@brief RSA encryption of suitably padded plaintext
 *
	@param PUB the input RSA public key
	@param F is input padded message
	@param G is the output ciphertext
 */
extern void MCL_RSA_ENCRYPT(MCL_rsa_public_key* PUB,mcl_octet *F,mcl_octet *G); 
/**	@brief RSA decryption of ciphertext
 *
	@param PRIV the input RSA private key
	@param G is the input ciphertext
	@param F is output plaintext (requires unpadding)

 */
extern void MCL_RSA_DECRYPT(MCL_rsa_private_key* PRIV,mcl_octet *G,mcl_octet *F);  
/**	@brief Destroy an RSA private Key
 *
	@param PRIV the input RSA private key. Destroyed on output.
 */
extern void MCL_RSA_PRIVATE_KEY_KILL(MCL_rsa_private_key *PRIV);

#endif
