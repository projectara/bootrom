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

/* ARA FF header file */
/* Designed for AES128 to AES256 security, 256 to 521 bit elliptic curves */

/**
 * @file mcl_ff.h
 * @author Mike Scott and Kealan McCusker
 * @date 19th May 2015
 * @brief Main Header File
 *
 * Allows some user configuration
 * defines structures
 * declares functions
 * 
 */

/* NOTE: There is normally only one user configurable section in this header - see below */

#ifndef MCL_FF_H
#define MCL_FF_H

#define MCL_HFLEN (MCL_FFLEN/2)

#include "mcl_oct.h"

/* Finite Field Prototypes */
/**	@brief Copy one FF element of given length to another
 *
	@param x FF instance to be copied to, on exit = y
	@param y FF instance to be copied from
	@param n size of FF in MCL_BIGs

 */
extern void MCL_FF_copy(mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],int n);
/**	@brief Initialize an FF element of given length from a 32-bit integer m
 *
	@param x FF instance to be copied to, on exit = m
	@param m integer
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_init(mcl_chunk x[][MCL_BS],sign32 m,int n);
/**	@brief Set FF element of given size to zero
 *
	@param x FF instance to be set to zero
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_zero(mcl_chunk x[][MCL_BS],int n);
/**	@brief Tests for FF element equal to zero
 *
	@param x FF number to be tested
	@param n size of FF in MCL_BIGs
	@return 1 if zero, else returns 0
 */
extern int MCL_FF_iszilch(mcl_chunk x[][MCL_BS],int n);
/**	@brief  return parity of an FF, that is the least significant bit
 *
	@param x FF number
	@return 0 or 1
 */
extern int MCL_FF_parity(mcl_chunk x[][MCL_BS]);
/**	@brief  return least significant m bits of an FF
 *
	@param x FF number
	@param m number of bits to return. Assumed to be less than MCL_BASEBITS.
	@return least significant n bits as an integer
 */
extern int MCL_FF_lastbits(mcl_chunk x[][MCL_BS],int m);
/**	@brief Set FF element of given size to unity
 *
	@param x FF instance to be set to unity
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_one(mcl_chunk x[][MCL_BS],int n);
/**	@brief Compares two FF numbers. Inputs must be normalised externally
 *
	@param x first FF number to be compared
	@param y second FF number to be compared
	@param n size of FF in MCL_BIGs
	@return -1 is x<y, 0 if x=y, 1 if x>y
 */
extern int MCL_FF_comp(mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],int n);
/**	@brief addition of two FFs
 *
	@param x FF instance, on exit = y+z 
	@param y FF instance
	@param z FF instance
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_add(mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],mcl_chunk z[][MCL_BS],int n);
/**	@brief subtraction of two FFs
 *
	@param x FF instance, on exit = y-z 
	@param y FF instance
	@param z FF instance
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_sub(mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],mcl_chunk z[][MCL_BS],int n);
/**	@brief increment an FF by an integer,and normalise
 *
	@param x FF instance, on exit = x+m
	@param m an integer to be added to x
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_inc(mcl_chunk x[][MCL_BS],int m,int n);
/**	@brief Decrement an FF by an integer,and normalise
 *
	@param x FF instance, on exit = x-m
	@param m an integer to be subtracted from x
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_dec(mcl_chunk x[][MCL_BS],int m,int n);
/**	@brief Normalises the components of an FF
 *
	@param x FF instance to be normalised
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_norm(mcl_chunk x[][MCL_BS],int n);
/**	@brief Shift left an FF by 1 bit
 *
	@param x FF instance to be shifted left
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_shl(mcl_chunk x[][MCL_BS],int n);
/**	@brief Shift right an FF by 1 bit
 *
	@param x FF instance to be shifted right
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_shr(mcl_chunk x[][MCL_BS],int n);
/**	@brief Formats and outputs an FF to the console
 *
	@param x FF instance to be printed
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_output(mcl_chunk x[][MCL_BS],int n);
/**	@brief Formats and outputs an FF instance to an mcl_octet string
 *
	Converts an FF to big-endian base 256 form.
	@param S output mcl_octet string
	@param x FF instance to be converted to an mcl_octet string
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_toOctet(mcl_octet *S,mcl_chunk x[][MCL_BS],int n);
/**	@brief Populates an FF instance from an mcl_octet string
 *
	Creates FF from big-endian base 256 form.
	@param x FF instance to be created from an mcl_octet string
	@param S input mcl_octet string
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_fromOctet(mcl_chunk x[][MCL_BS],mcl_octet *S,int n);
/**	@brief Multiplication of two FFs
 *
	Uses Karatsuba method internally
	@param x FF instance, on exit = y*z
	@param y FF instance
	@param z FF instance
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_mul(mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],mcl_chunk z[][MCL_BS],int n); 
/**	@brief Reduce FF mod a modulus
 *
	This is slow
	@param x FF instance to be reduced mod m - on exit = x mod m
	@param m FF modulus
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_mod(mcl_chunk x[][MCL_BS],mcl_chunk m[][MCL_BS],int n);
/**	@brief Square an FF
 *
	Uses Karatsuba method internally
	@param x FF instance, on exit = y^2
	@param y FF instance to be squared
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_sqr(mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],int n);
/**	@brief Reduces a double-length FF with respect to a given modulus
 *
	This is slow
	@param x FF instance, on exit = y mod z
	@param y FF instance, of double length 2*n
	@param z FF modulus
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_dmod(mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],mcl_chunk z[][MCL_BS],int n);
/**	@brief Invert an FF mod a prime modulus
 *
	@param x FF instance, on exit = 1/y mod z
	@param y FF instance
	@param z FF prime modulus
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_invmodp(mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],mcl_chunk z[][MCL_BS],int n);
/**	@brief Create an FF from a random number generator
 *
	@param x FF instance, on exit x is a random number of length n MCL_BIGs with most significant bit a 1
	@param R an instance of a Cryptographically Secure Random Number Generator
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_random(mcl_chunk x[][MCL_BS],csprng *R,int n);
/**	@brief Create a random FF less than a given modulus from a random number generator
 *
	@param x FF instance, on exit x is a random number < y
	@param y FF instance, the modulus
	@param R an instance of a Cryptographically Secure Random Number Generator
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_randomnum(mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],csprng *R,int n);
/**	@brief Calculate r=x^e mod m, side channel resistant
 *
	@param r FF instance, on exit = x^e mod p
	@param x FF instance
	@param e FF exponent
	@param m FF modulus
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_skpow(mcl_chunk r[][MCL_BS],mcl_chunk x[][MCL_BS],mcl_chunk e[][MCL_BS],mcl_chunk m[][MCL_BS],int n);
/**	@brief Calculate r=x^e mod m, side channel resistant
 *
	For short MCL_BIG exponent
	@param r FF instance, on exit = x^e mod p
	@param x FF instance
	@param e MCL_BIG exponent
	@param m FF modulus
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_skspow(mcl_chunk r[][MCL_BS],mcl_chunk x[][MCL_BS],MCL_BIG e,mcl_chunk m[][MCL_BS],int n);
/**	@brief Calculate r=x^e mod m
 *
	For very short integer exponent
	@param r FF instance, on exit = x^e mod p
	@param x FF instance
	@param e integer exponent
	@param m FF modulus
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_power(mcl_chunk r[][MCL_BS],mcl_chunk x[][MCL_BS],int e,mcl_chunk m[][MCL_BS],int n);
/**	@brief Calculate r=x^e mod m
 *
	@param r FF instance, on exit = x^e mod p
	@param x FF instance
	@param e FF exponent
	@param m FF modulus
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_pow(mcl_chunk r[][MCL_BS],mcl_chunk x[][MCL_BS],mcl_chunk e[][MCL_BS],mcl_chunk m[][MCL_BS],int n);
/**	@brief Test if an FF has factor in common with integer s
 *
	@param x FF instance to be tested
	@param s the supplied integer
	@param n size of FF in MCL_BIGs
	@return 1 if gcd(x,s)!=1, else return 0
 */
extern int MCL_FF_cfactor(mcl_chunk x[][MCL_BS],sign32 s,int n);
/**	@brief Test if an FF is prime
 *
	Uses Miller-Rabin Method
	@param x FF instance to be tested
	@param R an instance of a Cryptographically Secure Random Number Generator
	@param n size of FF in MCL_BIGs
	@return 1 if x is (almost certainly) prime, else return 0
 */
extern int MCL_FF_prime(mcl_chunk x[][MCL_BS],csprng *R,int n);
/**	@brief Calculate r=x^e.y^f mod m
 *
	@param r FF instance, on exit = x^e.y^f mod p
	@param x FF instance
	@param e MCL_BIG exponent
	@param y FF instance
	@param f MCL_BIG exponent
	@param m FF modulus
	@param n size of FF in MCL_BIGs
 */
extern void MCL_FF_pow2(mcl_chunk r[][MCL_BS],mcl_chunk x[][MCL_BS],MCL_BIG e,mcl_chunk y[][MCL_BS],MCL_BIG f,mcl_chunk m[][MCL_BS],int n);






#endif
