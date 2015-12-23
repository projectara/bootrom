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

/* ARA Octet header file */

/**
 * @file mcl_oct.h
 * @author Mike Scott and Kealan McCusker
 * @date 19th May 2015
 * @brief Main Header File
 *
 * defines structures
 * declares functions
 * 
 */

#ifndef MCL_OCT_H
#define MCL_OCT_H

#include "mcl_arch.h"
#include "mcl_rand.h"
/**
	@brief Portable representation of a big positive number
*/

typedef struct
{
    int len; /**< length in bytes  */
    int max; /**< max length allowed - enforce truncation  */
    char *val; /**< byte array  */
} mcl_octet;

/* Octet string handlers */
/**	@brief Formats and outputs an mcl_octet to the console in hex
 *
	@param O Octet to be output
 */
extern void MCL_OCT_output(mcl_octet *O);
/**	@brief Formats and outputs an mcl_octet to the console as a character string
 *
	@param O Octet to be output
 */
extern void MCL_OCT_output_string(mcl_octet *O);
/**	@brief Wipe clean an mcl_octet
 *
	@param O Octet to be cleaned
 */
extern void MCL_OCT_clear(mcl_octet *O);
/**	@brief Compare two mcl_octets
 *
	@param O first Octet to be compared
	@param P second Octet to be compared
	@return 1 if equal, else 0
 */
extern int  MCL_OCT_comp(mcl_octet *O,mcl_octet *P);
/**	@brief Compare first n bytes of two mcl_octets
 *
	@param O first Octet to be compared
	@param P second Octet to be compared
	@param n number of bytes to compare
	@return 1 if equal, else 0
 */
extern int  MCL_OCT_ncomp(mcl_octet *O,mcl_octet *P,int n);
/**	@brief Join from a C string to end of an mcl_octet
 *
	Truncates if there is no room
	@param O Octet to be written to
	@param s zero terminated string to be joined to mcl_octet
 */
extern void MCL_OCT_jstring(mcl_octet *O,char *s);
/**	@brief Join bytes to end of an mcl_octet
 *
	Truncates if there is no room
	@param O Octet to be written to
	@param s bytes to be joined to end of mcl_octet
	@param n number of bytes to join
 */
extern void MCL_OCT_jbytes(mcl_octet *O,char *s,int n);
/**	@brief Join single byte to end of an mcl_octet, repeated n times
 *
	Truncates if there is no room
	@param O Octet to be written to
	@param b byte to be joined to end of mcl_octet
	@param n number of times b is to be joined
 */
extern void MCL_OCT_jbyte(mcl_octet *O,int b,int n);
/**	@brief Join one mcl_octet to the end of another
 *
	Truncates if there is no room
	@param O Octet to be written to
	@param P Octet to be joined to the end of O
 */
extern void MCL_OCT_jmcl_octet(mcl_octet *O,mcl_octet *P);
/**	@brief XOR common bytes of a pair of Octets
 *
	@param O Octet - on exit = O xor P
	@param P Octet to be xored into O
 */
extern void MCL_OCT_xor(mcl_octet *O,mcl_octet *P);
/**	@brief reset Octet to zero length
 *
	@param O Octet to be emptied
 */
extern void MCL_OCT_empty(mcl_octet *O);
/**	@brief Pad out an Octet to the given length
 *
	Padding is done by inserting leading zeros, so abcd becomes 00abcd
	@param O Octet to be padded
	@param n new length of Octet
 */
extern int MCL_OCT_pad(mcl_octet *O,int n);
/**	@brief Convert an Octet to printable base64 number
 *
	@param b zero terminated byte array to take base64 conversion
	@param O Octet to be converted
 */
extern void MCL_OCT_tobase64(char *b,mcl_octet *O);
/**	@brief Populate an Octet from base64 number
 *
 	@param O Octet to be populated
	@param b zero terminated base64 string

 */
extern void MCL_OCT_frombase64(mcl_octet *O,char *b);
/**	@brief Copy one Octet into another
 *
 	@param O Octet to be copied to
	@param P Octet to be copied from

 */
extern void MCL_OCT_copy(mcl_octet *O,mcl_octet *P);
/**	@brief XOR every byte of an mcl_octet with input m
 *
 	@param O Octet 
	@param m byte to be XORed with every byte of O

 */
extern void MCL_OCT_xorbyte(mcl_octet *O,int m);
/**	@brief Chops Octet into two, leaving first n bytes in O, moving the rest to P
 *
 	@param O Octet to be chopped
	@param P new Octet to be created 
	@param n number of bytes to chop off O

 */
extern void MCL_OCT_chop(mcl_octet *O,mcl_octet *P,int n);
/**	@brief Join n bytes of integer m to end of Octet O (big endian)
 *
	Typically n is 4 for a 32-bit integer
 	@param O Octet to be appended to
	@param m integer to be appended to O
	@param n number of bytes in m

 */
extern void MCL_OCT_jint(mcl_octet *O,int m,int n);
/**	@brief Create an Octet from bytes taken from a random number generator
 *
	Truncates if there is no room
 	@param O Octet to be populated
	@param R an instance of a Cryptographically Secure Random Number Generator
	@param n number of bytes to extracted from R

 */
extern void MCL_OCT_rand(mcl_octet *O,csprng *R,int n);
/**	@brief Shifts Octet left by n bytes
 *
	Leftmost bytes disappear 
 	@param O Octet to be shifted
	@param n number of bytes to shift

 */
extern void MCL_OCT_shl(mcl_octet *O,int n);
/**	@brief Shifts Octet right by n bytes
 *
	Rightmost bytes set to zero 
 	@param O Octet to be shifted
	@param n number of bytes to shift

 */
extern void MCL_OCT_shr(mcl_octet *O,int n);
/**	@brief Convert an Octet to printable hex number
 *
	@param dst hex value
	@param src Octet to be converted
 */
extern void MCL_OCT_toHex(mcl_octet *src,char *dst);
/**	@brief Convert an Octet to string
 *
	@param dst string value
	@param src Octet to be converted
 */
extern void MCL_OCT_toStr(mcl_octet *src,char *dst);

#endif
