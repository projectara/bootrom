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

/* ARAcrypt header file */

/**
 * @file mcl_big.h
 * @author Mike Scott and Kealan McCusker
 * @date 19th May 2015
 * @brief Header File for MCL_BIG numbers
 *
 * defines structures
 * declares functions
 * 
 */

/* NOTE: There is normally only one user configurable section in this header - see below */

#ifndef MCL_BIG_H
#define MCL_BIG_H

#include "mcl_rand.h"

/*

Note that a normalised MCL_BIG consists of digits mod 2^MCL_BASEBITS
However MCL_BIG digits may be "extended" up to 2^(WORDLENGTH-1).

MCL_BIGs in extended form may need to be normalised before certain 
operations.

A MCL_BIG may be "reduced" to be less that the Modulus, or it 
may be "unreduced" and allowed to grow greater than the 
Modulus.

Normalisation is quite fast. Reduction involves conditional branches, 
which can be regarded as significant "speed bumps". We try to 
delay reductions as much as possible. Reductions may also involve 
side channel leakage, so delaying and batching them
hopefully disguises internal operations.

*/

/* MCL_BIG number prototypes */

/**	@brief Calculates a*b+c+*d
 *
	Calculate partial product of a.b, add in carry c, and add total to d
	@param a multiplier
	@param b multiplicand
	@param c carry
	@param d pointer to accumulated bottom half of result
	@return top half of result
 */
extern mcl_chunk MCL_muladd(mcl_chunk a,mcl_chunk b,mcl_chunk c,mcl_chunk *d);
/**	@brief Tests for MCL_BIG equal to zero
 *
	@param x a MCL_BIG number
	@return 1 if zero, else returns 0
 */
extern int MCL_BIG_iszilch(MCL_BIG x);
/**	@brief Tests for DMCL_BIG equal to zero
 *
	@param x a DMCL_BIG number
	@return 1 if zero, else returns 0
 */
extern int MCL_BIG_diszilch(DMCL_BIG x);
/**	@brief Outputs a MCL_BIG number to the console
 *
	@param x a MCL_BIG number
 */
extern void MCL_BIG_output(MCL_BIG x);
/**	@brief Outputs a MCL_BIG number to the console in raw form (for debugging)
 *
	@param x a MCL_BIG number
 */
extern void MCL_BIG_rawoutput(MCL_BIG x);
/**	@brief Conditional constant time swap of two MCL_BIG numbers
 *
	Conditionally swaps parameters in constant time (without branching)
	@param x a MCL_BIG number
	@param y another MCL_BIG number
	@param s swap takes place if not equal to 0
 */
extern void MCL_BIG_cswap(MCL_BIG x,MCL_BIG y,int s);
/**	@brief Conditional copy of MCL_BIG number
 *
	Conditionally copies second parameter to the first (without branching)
	@param x a MCL_BIG number
	@param y another MCL_BIG number
	@param s copy takes place if not equal to 0
 */
extern void MCL_BIG_cmove(MCL_BIG x,MCL_BIG y,int s);
/**	@brief Convert from MCL_BIG number to byte array
 *
	@param a byte array
	@param x MCL_BIG number
 */
extern void MCL_BIG_toBytes(char *a,MCL_BIG x);
/**	@brief Convert to MCL_BIG number from byte array
 *
	@param x MCL_BIG number
	@param a byte array
 */
extern void MCL_BIG_fromBytes(MCL_BIG x,char *a);
/**	@brief Convert to MCL_BIG number from byte array of given length
 *
	@param x MCL_BIG number
	@param a byte array
	@param s byte array length
 */
extern void MCL_BIG_fromBytesLen(MCL_BIG x,char *a,int s);
/**	@brief Outputs a DMCL_BIG number to the console
 *
	@param x a DMCL_BIG number
 */
extern void MCL_BIG_doutput(DMCL_BIG x);
/**	@brief Copy MCL_BIG from Read-Only Memory to a MCL_BIG
 *
	@param x MCL_BIG number
	@param y MCL_BIG number in ROM
 */
extern void MCL_BIG_rcopy(MCL_BIG x,const MCL_BIG y);
/**	@brief Copy MCL_BIG to another MCL_BIG
 *
	@param x MCL_BIG number
	@param y MCL_BIG number to be copied
 */
extern void MCL_BIG_copy(MCL_BIG x,MCL_BIG y);
/**	@brief Copy DMCL_BIG to another DMCL_BIG
 *
	@param x DMCL_BIG number
	@param y DMCL_BIG number to be copied
 */
extern void MCL_BIG_dcopy(DMCL_BIG x,DMCL_BIG y);
/**	@brief Copy MCL_BIG to upper half of DMCL_BIG
 *
	@param x DMCL_BIG number
	@param y MCL_BIG number to be copied
 */
extern void MCL_BIG_dsucopy(DMCL_BIG x,MCL_BIG y);
/**	@brief Copy MCL_BIG to lower half of DMCL_BIG
 *
	@param x DMCL_BIG number
	@param y MCL_BIG number to be copied
 */
extern void MCL_BIG_dscopy(DMCL_BIG x,MCL_BIG y);
/**	@brief Copy lower half of DMCL_BIG to a MCL_BIG
 *
	@param x MCL_BIG number
	@param y DMCL_BIG number to be copied
 */
extern void MCL_BIG_sdcopy(MCL_BIG x,DMCL_BIG y);
/**	@brief Copy upper half of DMCL_BIG to a MCL_BIG
 *
	@param x MCL_BIG number
	@param y DMCL_BIG number to be copied
 */
extern void MCL_BIG_sducopy(MCL_BIG x,DMCL_BIG y);
/**	@brief Set MCL_BIG to zero
 *
	@param x MCL_BIG number to be set to zero
 */
extern void MCL_BIG_zero(MCL_BIG x);
/**	@brief Set DMCL_BIG to zero
 *
	@param x DMCL_BIG number to be set to zero
 */
extern void MCL_BIG_dzero(DMCL_BIG x);
/**	@brief Set MCL_BIG to one (unity)
 *
	@param x MCL_BIG number to be set to one.
 */
extern void MCL_BIG_one(MCL_BIG x);
/**	@brief Set MCL_BIG to inverse mod 2^256
 *
	@param x MCL_BIG number to be inverted
 */
extern void MCL_BIG_invmod2m(MCL_BIG x);
/**	@brief Set MCL_BIG to sum of two MCL_BIGs - output not normalised
 *
	@param x MCL_BIG number, sum of other two
	@param y MCL_BIG number
	@param z MCL_BIG number
 */
extern void MCL_BIG_add(MCL_BIG x,MCL_BIG y,MCL_BIG z);
/**	@brief Increment MCL_BIG by a small integer - output not normalised
 *
	@param x MCL_BIG number to be incremented
	@param i integer
 */
extern void MCL_BIG_inc(MCL_BIG x,int i);
/**	@brief Set MCL_BIG to difference of two MCL_BIGs
 *
	@param x MCL_BIG number, difference of other two - output not normalised
	@param y MCL_BIG number
	@param z MCL_BIG number
 */
extern void MCL_BIG_sub(MCL_BIG x,MCL_BIG y,MCL_BIG z);
/**	@brief Decrement MCL_BIG by a small integer - output not normalised
 *
	@param x MCL_BIG number to be decremented
	@param i integer
 */
extern void MCL_BIG_dec(MCL_BIG x,int i);
/**	@brief Set DMCL_BIG to difference of two DMCL_BIGs
 *
	@param x DMCL_BIG number, difference of other two - output not normalised
	@param y DMCL_BIG number
	@param z DMCL_BIG number
 */
extern void MCL_BIG_dsub(DMCL_BIG x,DMCL_BIG y,DMCL_BIG z);
/**	@brief Multiply MCL_BIG by a small integer - output not normalised
 *
	@param x MCL_BIG number, product of other two
	@param y MCL_BIG number
	@param i small integer
 */
extern void MCL_BIG_imul(MCL_BIG x,MCL_BIG y,int i);
/**	@brief Multiply MCL_BIG by not-so-small small integer - output normalised
 *
	@param x MCL_BIG number, product of other two
	@param y MCL_BIG number
	@param i small integer
	@return Overflowing bits
 */
extern mcl_chunk MCL_BIG_pmul(MCL_BIG x,MCL_BIG y,int i);
/**	@brief Divide MCL_BIG by 3 - output normalised
 *
	@param x MCL_BIG number
	@return Remainder
 */
extern int MCL_BIG_div3(MCL_BIG x);
/**	@brief Multiply MCL_BIG by even bigger small integer resulting in a DMCL_BIG - output normalised
 *
	@param x DMCL_BIG number, product of other two
	@param y MCL_BIG number
	@param i small integer
 */
extern void MCL_BIG_pxmul(DMCL_BIG x,MCL_BIG y,int i);
/**	@brief Multiply MCL_BIG by another MCL_BIG resulting in DMCL_BIG - inputs normalised and output normalised
 *
	@param x DMCL_BIG number, product of other two
	@param y MCL_BIG number
	@param z MCL_BIG number
 */
extern void MCL_BIG_mul(DMCL_BIG x,MCL_BIG y,MCL_BIG z);
/**	@brief Multiply MCL_BIG by another MCL_BIG resulting in another MCL_BIG - inputs normalised and output normalised
 *
	Note that the product must fit into a MCL_BIG, and x must be distinct from y and z
	@param x MCL_BIG number, product of other two
	@param y MCL_BIG number
	@param z MCL_BIG number
 */
extern void MCL_BIG_smul(MCL_BIG x,MCL_BIG y,MCL_BIG z);
/**	@brief Square MCL_BIG resulting in a DMCL_BIG - input normalised and output normalised
 *
	@param x DMCL_BIG number, square of a MCL_BIG
	@param y MCL_BIG number to be squared
 */
extern void MCL_BIG_sqr(DMCL_BIG x,MCL_BIG y);
/**	@brief Shifts a MCL_BIG left by any number of bits - input must be normalised, output normalised
 *
	@param x MCL_BIG number to be shifted
	@param s Number of bits to shift
 */
extern void MCL_BIG_shl(MCL_BIG x,int s);
/**	@brief Fast shifts a MCL_BIG left by a small number of bits - input must be normalised, output will be normalised
 *
	The number of bits to be shifted must be less than MCL_BASEBITS
	@param x MCL_BIG number to be shifted
	@param s Number of bits to shift
	@return Overflow bits
 */
extern mcl_chunk MCL_BIG_fshl(MCL_BIG x,int s);
/**	@brief Shifts a DMCL_BIG left by any number of bits - input must be normalised, output normalised
 *
	@param x DMCL_BIG number to be shifted
	@param s Number of bits to shift
 */
extern void MCL_BIG_dshl(DMCL_BIG x,int s);
/**	@brief Shifts a MCL_BIG right by any number of bits - input must be normalised, output normalised
 *
	@param x MCL_BIG number to be shifted
	@param s Number of bits to shift
 */
extern void MCL_BIG_shr(MCL_BIG x,int s);
/**	@brief Fast shifts a MCL_BIG right by a small number of bits - input must be normalised, output will be normalised
 *
	The number of bits to be shifted must be less than MCL_BASEBITS
	@param x MCL_BIG number to be shifted
	@param s Number of bits to shift
	@return Shifted out bits
 */
extern mcl_chunk MCL_BIG_fshr(MCL_BIG x,int s);
/**	@brief Shifts a DMCL_BIG right by any number of bits - input must be normalised, output normalised
 *
	@param x DMCL_BIG number to be shifted
	@param s Number of bits to shift
 */
extern void MCL_BIG_dshr(DMCL_BIG x,int s);
/**	@brief Splits a DMCL_BIG into two MCL_BIGs - input must be normalised, outputs normalised
 *
	Internal function. The value of s must be approximately in the middle of the DMCL_BIG. 
	Typically used to extract z mod 2^MODBITS and z/2^MODBITS
	@param x MCL_BIG number, top half of z
	@param y MCL_BIG number, bottom half of z
	@param z DMCL_BIG number to be split in two.
	@param s Bit position at which to split
 */
extern void MCL_BIG_split(MCL_BIG x,MCL_BIG y,DMCL_BIG z,int s);
/**	@brief Normalizes a MCL_BIG number - output normalised
 *
	All digits of the input MCL_BIG are reduced mod 2^MCL_BASEBITS
	@param x MCL_BIG number to be normalised
 */
extern mcl_chunk MCL_BIG_norm(MCL_BIG x);
/**	@brief Normalizes a DMCL_BIG number - output normalised
 *
	All digits of the input DMCL_BIG are reduced mod 2^MCL_BASEBITS
	@param x DMCL_BIG number to be normalised
 */
extern void MCL_BIG_dnorm(DMCL_BIG x);
/**	@brief Compares two MCL_BIG numbers. Inputs must be normalised externally
 *
	@param x first MCL_BIG number to be compared
	@param y second MCL_BIG number to be compared
	@return -1 is x<y, 0 if x=y, 1 if x>y
 */
extern int MCL_BIG_comp(MCL_BIG x,MCL_BIG y);
/**	@brief Compares two DMCL_BIG numbers. Inputs must be normalised externally
 *
	@param x first DMCL_BIG number to be compared
	@param y second DMCL_BIG number to be compared
	@return -1 is x<y, 0 if x=y, 1 if x>y
 */
extern int MCL_BIG_dcomp(DMCL_BIG x,DMCL_BIG y);
/**	@brief Calculate number of bits in a MCL_BIG - output normalised
 *
	@param x MCL_BIG number
	@return Number of bits in x
 */
extern int MCL_BIG_nbits(MCL_BIG x);
/**	@brief Calculate number of bits in a DMCL_BIG - output normalised
 *
	@param x DMCL_BIG number
	@return Number of bits in x
 */
extern int MCL_BIG_dnbits(DMCL_BIG x);
/**	@brief Reduce x mod n - input and output normalised
 *
	Slow but rarely used
	@param x MCL_BIG number to be reduced mod n
	@param n The modulus
 */
extern void MCL_BIG_mod(MCL_BIG x,MCL_BIG n);
/**	@brief Divide x by n - output normalised
 *
	Slow but rarely used
	@param x MCL_BIG number to be divided by n
	@param n The Divisor
 */
extern void MCL_BIG_sdiv(MCL_BIG x,MCL_BIG n);
/**	@brief  x=y mod n - output normalised
 *
	Slow but rarely used. y is destroyed.
	@param x MCL_BIG number, on exit = y mod n
	@param y DMCL_BIG number
	@param n MCL_Modulus
 */
extern void MCL_BIG_dmod(MCL_BIG x,DMCL_BIG y,MCL_BIG n);
/**	@brief  x=y/n - output normalised
 *
	Slow but rarely used. y is destroyed.
	@param x MCL_BIG number, on exit = y/n
	@param y DMCL_BIG number
	@param n MCL_Modulus
 */
extern void MCL_BIG_ddiv(MCL_BIG x,DMCL_BIG y,MCL_BIG n);
/**	@brief  return parity of MCL_BIG, that is the least significant bit
 *
	@param x MCL_BIG number
	@return 0 or 1
 */
extern int MCL_BIG_parity(MCL_BIG x);
/**	@brief  return i-th of MCL_BIG
 *
	@param x MCL_BIG number
	@param i the bit of x to be returned
	@return 0 or 1
 */
extern int MCL_BIG_bit(MCL_BIG x,int i);
/**	@brief  return least significant bits of a MCL_BIG
 *
	@param x MCL_BIG number
	@param n number of bits to return. Assumed to be less than MCL_BASEBITS.
	@return least significant n bits as an integer
 */
extern int MCL_BIG_lastbits(MCL_BIG x,int n);
/**	@brief  Create a random MCL_BIG from a random number generator
 *
	Assumes that the random number generator has been suitably initialised
	@param x MCL_BIG number, on exit a random number
	@param r A pointer to a Cryptographically Secure Random Number Generator
 */
extern void MCL_BIG_random(MCL_BIG x,csprng *r);
/**	@brief  Create an unbiased random MCL_BIG from a random number generator, reduced with respect to a modulus
 *
	Assumes that the random number generator has been suitably initialised
	@param x MCL_BIG number, on exit a random number
	@param n The modulus
	@param r A pointer to a Cryptographically Secure Random Number Generator
 */
extern void MCL_BIG_randomnum(MCL_BIG x,MCL_BIG n,csprng *r);
/**	@brief  return NAF (Non-Adjacent-Form) value as +/- 1, 3 or 5, inputs must be normalised
 *
	Given x and 3*x extracts NAF value from given bit position, and returns number of bits processed, and number of trailing zeros detected if any
	@param x MCL_BIG number
	@param x3 MCL_BIG number, three times x
	@param i bit position 
	@param nbs pointer to integer returning number of bits processed
	@param nzs pointer to integer returning number of trailing 0s
	@return + or - 1, 3 or 5
 */
extern int MCL_BIG_nafbits(MCL_BIG x,MCL_BIG x3,int i,int *nbs,int *nzs);
/**	@brief  Calculate x=y*z mod n
 *
	Slow method for modular multiplication
	@param x MCL_BIG number, on exit = y*z mod n
	@param y MCL_BIG number
	@param z MCL_BIG number
	@param n The MCL_BIG MCL_Modulus
 */
extern void MCL_BIG_modmul(MCL_BIG x,MCL_BIG y,MCL_BIG z,MCL_BIG n);
/**	@brief  Calculate x=y/z mod n
 *
	Slow method for modular division
	@param x MCL_BIG number, on exit = y/z mod n
	@param y MCL_BIG number
	@param z MCL_BIG number
	@param n The MCL_BIG MCL_Modulus
 */
extern void MCL_BIG_moddiv(MCL_BIG x,MCL_BIG y,MCL_BIG z,MCL_BIG n);
/**	@brief  Calculate x=y^2 mod n
 *
	Slow method for modular squaring
	@param x MCL_BIG number, on exit = y^2 mod n
	@param y MCL_BIG number
	@param n The MCL_BIG MCL_Modulus
 */
extern void MCL_BIG_modsqr(MCL_BIG x,MCL_BIG y,MCL_BIG n);
/**	@brief  Calculate x=-y mod n
 *
	Modular negation
	@param x MCL_BIG number, on exit = -y mod n
	@param y MCL_BIG number
	@param n The MCL_BIG MCL_Modulus
 */
extern void MCL_BIG_modneg(MCL_BIG x,MCL_BIG y,MCL_BIG n);
/**	@brief  Calculate jacobi Symbol (x/y)
 *
	@param x MCL_BIG number
	@param y MCL_BIG number
	@return Jacobi symbol, -1,0 or 1
 */
extern int MCL_BIG_jacobi(MCL_BIG x,MCL_BIG y);
/**	@brief  Calculate x=1/y mod n
 *
	Modular Inversion - This is slow. Uses binary method.
	@param x MCL_BIG number, on exit = 1/y mod n
	@param y MCL_BIG number
	@param n The MCL_BIG MCL_Modulus
 */
extern void MCL_BIG_invmodp(MCL_BIG x,MCL_BIG y,MCL_BIG n);


#endif
