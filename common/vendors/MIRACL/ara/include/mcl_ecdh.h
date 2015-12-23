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
 * @file mcl_ecdh.h
 * @author Mike Scott and Kealan McCusker
 * @date 2nd June 2015
 * @brief ECDH Header file for implementation of standard EC protocols
 *
 * declares functions
 * 
 */

#ifndef MCL_ECDH_H
#define MCL_ECDH_H

#include "mcl_arch.h"
#include "mcl_config.h"
#include "mcl_rand.h"
#include "mcl_oct.h"
#include "mcl_hash.h"
#include "mcl_aes.h"
#include "mcl_big.h"
#include "mcl_ecp.h"
#include "mcl_fp.h"

#define MCL_MODBYTES (1+(MCL_MBITS-1)/8) /**< Number of bytes in MCL_Modulus */

#define MCL_EAS 16 /**< AES Key size in bytes - 16 for 128 bits, 32 for 256 bits */

#define MCL_EGS (1+(MCL_MBITS-1)/8)  /**< ECC Group Size in bytes */
#define MCL_EFS (1+(MCL_MBITS-1)/8)  /**< ECC Field Size in bytes */
#define MCL_HASH_TYPE_ECC MCL_SHA256 /**< Hash Length in bytes for ECC */

#define MCL_ECDH_OK                     0     /**< Function completed without error */
/*#define MCL_ECDH_DOMAIN_ERROR          -1*/
#define MCL_ECDH_INVALID_PUBLIC_KEY    -2	/**< Public Key is Invalid */
#define MCL_ECDH_ERROR                 -3	/**< ECDH Internal Error */
#define MCL_ECDH_INVALID               -4	/**< ECDH Internal Error */

/* ECDH Auxiliary Functions */

/**	@brief Initialise a random number generator
 *
	@param R is a pointer to a cryptographically secure random number generator
	@param S is an input truly random seed value
 */
extern void MCL_CREATE_CSPRNG(csprng *R,mcl_octet *S);
/**	@brief Kill a random number generator
 *
	Deletes all internal state
	@param R is a pointer to a cryptographically secure random number generator
 */
extern void MCL_KILL_CSPRNG(csprng *R);
/**	@brief hash an mcl_octet into another mcl_octet
 *
 	@param h is the hash type
	@param I input mcl_octet
	@param O output mcl_octet - H(I)
 */
extern void MCL_HASH(int h,mcl_octet *I,mcl_octet *O);
/**	@brief MCL_HMAC of message M using key K to create tag of length len in mcl_octet tag
 *
	IEEE-1363 MAC1 function. Uses SHA256 internally.
	@param h is the hash type
	@param M input message mcl_octet
	@param K input encryption key
	@param len is output desired length of MCL_HMAC tag
	@param tag is the output MCL_HMAC
	@return 0 for bad parameters, else 1
 */
extern int MCL_HMAC(int h,mcl_octet *M,mcl_octet *K,int len,mcl_octet *tag);

/*extern void KDF1(mcl_octet *,int,mcl_octet *);*/

/**	@brief Key Derivation Function - generates key K from inputs Z and P
 *
	IEEE-1363 MCL_KDF2 Key Derivation Function. Uses SHA256 internally.
	@param h is the hash type
	@param Z input mcl_octet
	@param P input key derivation parameters - can be NULL
	@param len is output desired length of key
	@param K is the derived key
 */
extern void MCL_KDF2(int h,mcl_octet *Z,mcl_octet *P,int len,mcl_octet *K);
/**	@brief Password Based Key Derivation Function - generates key K from password, salt and repeat counter
 *
	MCL_PBKDF2 Password Based Key Derivation Function. Uses SHA256 internally.
	@param h is the hash type
	@param P input password
	@param S input salt
	@param rep Number of times to be iterated.
	@param len is output desired length of key
	@param K is the derived key
 */
extern void MCL_PBKDF2(int h,mcl_octet *P,mcl_octet *S,int rep,int len,mcl_octet *K);
/**	@brief AES encrypts a plaintext to a ciphtertext
 *
	IEEE-1363 MCL_AES_CBC_IV0_ENCRYPT function. Encrypts in CBC mode with a zero IV, padding as necessary to create a full final block.
	@param K AES key
	@param P input plaintext mcl_octet
	@param C output ciphertext mcl_octet
 */
extern void MCL_AES_CBC_IV0_ENCRYPT(mcl_octet *K,mcl_octet *P,mcl_octet *C);
/**	@brief AES encrypts a plaintext to a ciphtertext
 *
	IEEE-1363 MCL_AES_CBC_IV0_DECRYPT function. Decrypts in CBC mode with a zero IV.
	@param K AES key
	@param C input ciphertext mcl_octet
	@param P output plaintext mcl_octet
	@return 0 if bad input, else 1
 */
extern int MCL_AES_CBC_IV0_DECRYPT(mcl_octet *K,mcl_octet *C,mcl_octet *P);

/* ECDH primitives - support functions */
/**	@brief Generate an ECC public/private key pair
 *
	@param R is a pointer to a cryptographically secure random number generator
	@param s the private key, an output internally randomly generated if R!=NULL, otherwise must be provided as an input
	@param W the output public key, which is s.G, where G is a fixed generator
	@return 0 or an error code
 */
extern int  MCL_ECP_KEY_PAIR_GENERATE(csprng *R,mcl_octet *s,mcl_octet *W);
/**	@brief Validate an ECC public key
 *
	@param f if = 0 just does some simple checks, else tests that W is of the correct order
	@param W the input public key to be validated
	@return 0 if public key is OK, or an error code
 */
extern int  MCL_ECP_PUBLIC_KEY_VALIDATE(int f,mcl_octet *W);

/* ECDH primitives */

/**	@brief Generate Diffie-Hellman shared key
 *
	IEEE-1363 Diffie-Hellman shared secret calculation
	@param s is the input private key,
	@param W the input public key of the other party
	@param K the output shared key, in fact the x-coordinate of s.W
	@return 0 or an error code
 */
extern int MCL_ECPSVDP_DH(mcl_octet *s,mcl_octet *W,mcl_octet *K);

/*#if MCL_CURVETYPE!=MCL_MONTGOMERY */
/* ECIES functions */
/**	@brief ECIES Encryption
 *
	IEEE-1363 ECIES Encryption
	@param h is the hash type
	@param P1 input Key Derivation parameters
	@param P2 input Encoding parameters
	@param R is a pointer to a cryptographically secure random number generator
	@param W the input public key of the recieving party
	@param M is the plaintext message to be encrypted
	@param len the length of the MCL_HMAC tag
	@param V component of the output ciphertext
	@param C the output ciphertext
	@param T the output MCL_HMAC tag, part of the ciphertext
 */
extern void MCL_ECP_ECIES_ENCRYPT(int h,mcl_octet *P1,mcl_octet *P2,csprng *R,mcl_octet *W,mcl_octet *M,int len,mcl_octet *V,mcl_octet *C,mcl_octet *T);
/**	@brief ECIES Decryption
 *
	IEEE-1363 ECIES Decryption
	@param h is the hash type
	@param P1 input Key Derivation parameters
	@param P2 input Encoding parameters
	@param V component of the input ciphertext
	@param C the input ciphertext
	@param T the input MCL_HMAC tag, part of the ciphertext
	@param U the input private key for decryption
	@param M the output plaintext message
	@return 1 if successful, else 0
 */
extern int MCL_ECP_ECIES_DECRYPT(int h,mcl_octet *P1,mcl_octet *P2,mcl_octet *V,mcl_octet *C,mcl_octet *T,mcl_octet *U,mcl_octet *M);

/* ECDSA functions */
/**	@brief ECDSA Signature
 *
	IEEE-1363 ECDSA Signature
	@param h is the hash type
	@param R is a pointer to a cryptographically secure random number generator
	@param s the input private signing key
	@param M the input message to be signed
	@param c component of the output signature
	@param d component of the output signature

 */
extern int MCL_ECPSP_DSA(int h,csprng *R,mcl_octet *s,mcl_octet *M,mcl_octet *c,mcl_octet *d);
/**	@brief ECDSA Signature Verification
 *
	IEEE-1363 ECDSA Signature Verification
	@param h is the hash type
	@param W the input public key
	@param M the input message
	@param c component of the input signature
	@param d component of the input signature
	@return 0 or an error code
 */
extern int MCL_ECPVP_DSA(int h,mcl_octet *W,mcl_octet *M,mcl_octet *c,mcl_octet *d);
/*#endif*/

#endif

