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

/* ARA AES encryption header file */
/* Designed for AES128 to AES256 security, 256 to 521 bit elliptic curves */

/**
 * @file mcl_aes.h
 * @author Mike Scott and Kealan McCusker
 * @date 19th May 2015
 * @brief Main Header File
 *
 * Allows some user configuration
 * defines structures
 * declares functions
 * 
 */

#ifndef MCL_AES_H
#define MCL_AES_H

/* Symmetric Encryption AES structure */

#define ECB   0  /**< Electronic Code Book */
#define CBC   1  /**< Cipher Block Chaining */
#define CFB1  2  /**< Cipher Feedback - 1 byte */
#define CFB2  3  /**< Cipher Feedback - 2 bytes */
#define CFB4  5  /**< Cipher Feedback - 4 bytes */
#define OFB1  14 /**< Output Feedback - 1 byte */
#define OFB2  15 /**< Output Feedback - 2 bytes */
#define OFB4  17 /**< Output Feedback - 4 bytes */
#define OFB8  21 /**< Output Feedback - 8 bytes */
#define OFB16 29 /**< Output Feedback - 16 bytes */

/**
	@brief AES instance
*/

typedef struct {
int Nk,Nr;
int mode;          /**< AES mode of operation */
unsign32 fkey[60]; /**< subkeys for encrypton */
unsign32 rkey[60]; /**< subkeys for decrypton */
char f[16];        /**< buffer for chaining vector */
} mcl_aes;

/* AES functions */
/**	@brief Reset AES mode or IV
 *
	@param A an instance of the AES
	@param m is the new active mode of operation (ECB, CBC, OFB, CFB etc)
	@param iv the new Initialisation Vector
 */
extern void MCL_AES_reset(mcl_aes *A,int m,char *iv);
/**	@brief Extract chaining vector from AES instance
 *
	@param A an instance of the AES
	@param f the extracted chaining vector
 */
extern void MCL_AES_getreg(mcl_aes *A,char * f);
/**	@brief Initialise an instance of AES and its mode of operation
 *
	@param A an instance AES
	@param m is the active mode of operation (ECB, CBC, OFB, CFB etc)
	@param n is the key length in bytes, 16, 24 or 32
	@param k the AES key as an array of 16 bytes
	@param iv the Initialisation Vector
	@return 0 for invalid n
 */
extern int MCL_AES_init(mcl_aes *A,int m,int n,char *k,char *iv);
/**	@brief Encrypt a single 16 byte block in ECB mode
 *
	@param A an instance of the AES
	@param b is an array of 16 plaintext bytes, on exit becomes ciphertext
 */
extern void MCL_AES_ecb_encrypt(mcl_aes *A,uchar * b);
/**	@brief Decrypt a single 16 byte block in ECB mode
 *
	@param A an instance of the AES
	@param b is an array of 16 cipherext bytes, on exit becomes plaintext
 */
extern void MCL_AES_ecb_decrypt(mcl_aes *A,uchar * b);
/**	@brief Encrypt a single 16 byte block in active mode
 *
	@param A an instance of the AES
	@param b is an array of 16 plaintext bytes, on exit becomes ciphertext
	@return 0, or overflow bytes from CFB mode
 */
extern unsign32 MCL_AES_encrypt(mcl_aes *A,char *b );
/**	@brief Decrypt a single 16 byte block in active mode
 *
	@param A an instance of the AES
	@param b is an array of 16 ciphertext bytes, on exit becomes plaintext
	@return 0, or overflow bytes from CFB mode
 */
extern unsign32 MCL_AES_decrypt(mcl_aes *A,char *b);
/**	@brief Clean up after application of AES
 *
	@param A an instance of the AES
 */
extern void MCL_AES_end(mcl_aes *A);


#endif

