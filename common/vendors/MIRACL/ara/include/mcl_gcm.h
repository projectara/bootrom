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

/* ARA GCM encryption header file */
/* Designed for AES128 to AES256 security, 256 to 521 bit elliptic curves */

/**
 * @file mcl_gcm.h
 * @author Mike Scott and Kealan McCusker
 * @date 19th May 2015
 * @brief Main Header File
 *
 * Allows some user configuration
 * defines structures
 * declares functions
 * 
 */

#ifndef MCL_GCM_H
#define MCL_GCM_H

#include "mcl_aes.h"

/* AES-GCM suppport.  */

#define MCL_GCM_ACCEPTING_HEADER 0 /**< GCM status */
#define MCL_GCM_ACCEPTING_CIPHER 1 /**< GCM status */
#define MCL_GCM_NOT_ACCEPTING_MORE 2 /**< GCM status */
#define MCL_GCM_FINISHED 3 /**< GCM status */
#define MCL_GCM_ENCRYPTING 0 /**< GCM mode */
#define MCL_GCM_DECRYPTING 1 /**< GCM mode */

/**
	@brief GCM mode instance, using AES internally
*/

typedef struct {
unsign32 table[128][4]; /**< 2k byte table */
uchar stateX[16];	/**< GCM Internal State */
uchar Y_0[16];		/**< GCM Internal State */
unsign32 lenA[2];	/**< GCM 64-bit length of header */
unsign32 lenC[2];	/**< GCM 64-bit length of ciphertext */
int status;		/**< GCM Status */
mcl_aes a;			/**< Internal Instance of AES cipher */
} mcl_gcm;

/* AES-GCM functions */
/**	@brief Initialise an instance of AES-GCM mode
 *
	@param G an instance AES-GCM
	@param nk is the key length in bytes, 16, 24 or 32
	@param k the AES key as an array of 16 bytes
	@param n the number of bytes in the Initialisation Vector (IV)
	@param iv the IV
 */
extern void MCL_GCM_init(mcl_gcm *G,int nk,char *k,int n,char *iv);
/**	@brief Add header (material to be authenticated but not encrypted)
 *
	Note that this function can be called any number of times with n a multiple of 16, and then one last time with any value for n
	@param G an instance AES-GCM
	@param b is the header material to be added
	@param n the number of bytes in the header
 */
extern int MCL_GCM_add_header(mcl_gcm *G,char *b,int n);
/**	@brief Add plaintext and extract ciphertext
 *
	Note that this function can be called any number of times with n a multiple of 16, and then one last time with any value for n
	@param G an instance AES-GCM
	@param c is the ciphertext generated
	@param p is the plaintext material to be added
	@param n the number of bytes in the plaintext
 */
extern int MCL_GCM_add_plain(mcl_gcm *G,char *c,char *p,int n);
/**	@brief Add ciphertext and extract plaintext
 *
	Note that this function can be called any number of times with n a multiple of 16, and then one last time with any value for n
	@param G an instance AES-GCM
	@param p is the plaintext generated
	@param c is the ciphertext material to be added
	@param n the number of bytes in the ciphertext
 */
extern int MCL_GCM_add_cipher(mcl_gcm *G,char *p,char *c,int n);
/**	@brief Finish off and extract authentication tag (MCL_HMAC)
 *
	@param G is an active instance AES-GCM
	@param t is the output 16 byte authentication tag
 */
extern void MCL_GCM_finish(mcl_gcm *G,char *t);

#endif
