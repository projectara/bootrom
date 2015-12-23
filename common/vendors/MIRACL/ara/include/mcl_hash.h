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

/* ARA Hashing header file */
/* Designed for AES128 to AES256 security, 256 to 521 bit elliptic curves */

/**
 * @file mcl_hash.h
 * @author Mike Scott and Kealan McCusker
 * @date 19th May 2015
 * @brief Main Header File
 *
 * Allows some user configuration
 * defines structures
 * declares functions
 * 
 */

#ifndef MCL_HASH_H
#define MCL_HASH_H

#define MCL_SHA1 20  /**< SHA-1 hashing */
#define MCL_SHA256 32 /**< SHA-256 hashing */
#define MCL_SHA384 48 /**< SHA-384 hashing */
#define MCL_SHA512 64 /**< SHA-512 hashing */

/**
	@brief SHA256/384/512 hash function instance
*/

typedef struct {
unsign32 length[2];  /**< 64-bit input length */
unsign32 h[8];       /**< Internal state */
unsign32 w[80];		/**< Internal state */
int hlen;			/**< Hash length in bytes */
} mcl_hash256;

typedef struct {
unsign64 length[2];  /**< 64-bit input length */
unsign64 h[8];       /**< Internal state */
unsign64 w[80];		/**< Internal state */
int hlen;           /**< Hash length in bytes */
} mcl_hash512;

typedef mcl_hash256 mcl_hash160;
typedef mcl_hash512 mcl_hash384;

/* Hash function */
/**	@brief Initialise an instance of SHA1
 *
	@param H an instance SHA1
 */
extern void MCL_HASH160_init(mcl_hash160 *H);
/**	@brief Add a byte to the hash
 *
	@param H an instance SHA1
	@param b byte to be included in hash
 */
extern void MCL_HASH160_process(mcl_hash160 *H,int b);
/**	@brief Generate 20-byte hash
 *
	@param H an instance SHA1
	@param h is the output 20-byte hash
 */
extern void MCL_HASH160_hash(mcl_hash160 *H,char *h);


/* Hash function */
/**	@brief Initialise an instance of SHA256
 *
	@param H an instance SHA256
 */
extern void MCL_HASH256_init(mcl_hash256 *H);
/**	@brief Add a byte to the hash
 *
	@param H an instance SHA256
	@param b byte to be included in hash
 */
extern void MCL_HASH256_process(mcl_hash256 *H,int b);
/**	@brief Generate 32-byte hash
 *
	@param H an instance SHA256
	@param h is the output 32-byte hash
 */
extern void MCL_HASH256_hash(mcl_hash256 *H,char *h);


/**	@brief Initialise an instance of SHA384
 *
	@param H an instance SHA384
 */
extern void MCL_HASH384_init(mcl_hash384 *H);
/**	@brief Add a byte to the hash
 *
	@param H an instance SHA384
	@param b byte to be included in hash
 */
extern void MCL_HASH384_process(mcl_hash384 *H,int b);
/**	@brief Generate 48-byte hash
 *
	@param H an instance SHA384
	@param h is the output 48-byte hash
 */
extern void MCL_HASH384_hash(mcl_hash384 *H,char *h);


/**	@brief Initialise an instance of SHA512
 *
	@param H an instance SHA512
 */
extern void MCL_HASH512_init(mcl_hash512 *H);
/**	@brief Add a byte to the hash
 *
	@param H an instance SHA512
	@param b byte to be included in hash
 */
extern void MCL_HASH512_process(mcl_hash512 *H,int b);
/**	@brief Generate 64-byte hash
 *
	@param H an instance SHA512
	@param h is the output 64-byte hash
 */
extern void MCL_HASH512_hash(mcl_hash512 *H,char *h);

#endif
