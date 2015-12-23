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

/* ARA FP header file */
/* Designed for AES128 to AES256 security, 256 to 521 bit elliptic curves */

/**
 * @file mcl_fp.h
 * @author Mike Scott and Kealan McCusker
 * @date 19th May 2015
 * @brief Main Header File
 * defines structures
 * declares functions
 * 
 */

/* NOTE: There is normally only one user configurable section in this header - see below */

#ifndef MCL_FP_H
#define MCL_FP_H

/* Field Params - see rom.c */
extern const mcl_chunk MCL_Modulus[];  /**< Actual MCL_Modulus set in rom.c */
extern const mcl_chunk MCL_MConst; /**< Montgomery only - 1/p mod 2^MCL_BASEBITS */

/* FP prototypes */

/**	@brief Tests for MCL_BIG equal to zero mod MCL_Modulus
 *
	@param x MCL_BIG number to be tested
	@return 1 if zero, else returns 0
 */
extern int MCL_FP_iszilch(MCL_BIG x);
/**	@brief Converts from MCL_BIG integer to n-residue form mod MCL_Modulus
 *
	@param x MCL_BIG number to be converted
 */
extern void MCL_FP_nres(MCL_BIG x);
/**	@brief Converts from n-residue form back to MCL_BIG integer form
 *
	@param x MCL_BIG number to be converted
 */
extern void MCL_FP_redc(MCL_BIG x);
/**	@brief Sets MCL_BIG to representation of unity in n-residue form
 *
	@param x MCL_BIG number to be set equal to unity.
 */
extern void MCL_FP_one(MCL_BIG x);
/**	@brief Reduces DMCL_BIG to MCL_BIG exploiting special form of the modulus
 *
	This function comes in different flavours depending on the form of MCL_Modulus that is currently in use.
	@param x MCL_BIG number, on exit = y mod MCL_Modulus
	@param y DMCL_BIG number to be reduced
 */
extern void MCL_FP10_mod(MCL_BIG x,DMCL_BIG y);
/**	@brief Fast Modular multiplication of two MCL_BIGs in n-residue form, mod MCL_Modulus
 *
	Uses appropriate fast modular reduction method
	@param x MCL_BIG number, on exit the modular product = y*z mod MCL_Modulus
	@param y MCL_BIG number, the multiplicand
	@param z MCL_BIG number, the multiplier
 */
extern void MCL_FP_mul(MCL_BIG x,MCL_BIG y,MCL_BIG z);
/**	@brief Fast Modular multiplication of a MCL_BIG in n-residue form, by a small integer, mod MCL_Modulus
 *
	@param x MCL_BIG number, on exit the modular product = y*i mod MCL_Modulus
	@param y MCL_BIG number, the multiplicand
	@param i a small number, the multiplier
 */
extern void MCL_FP_imul(MCL_BIG x,MCL_BIG y,int i);
/**	@brief Fast Modular squaring of a MCL_BIG in n-residue form, mod MCL_Modulus
 *
	Uses appropriate fast modular reduction method
	@param x MCL_BIG number, on exit the modular product = y^2 mod MCL_Modulus
	@param y MCL_BIG number, the number to be squared

 */
extern void MCL_FP_sqr(MCL_BIG x,MCL_BIG y);
/**	@brief Modular addition of two MCL_BIGs in n-residue form, mod MCL_Modulus
 *
	@param x MCL_BIG number, on exit the modular sum = y+z mod MCL_Modulus
	@param y MCL_BIG number
	@param z MCL_BIG number
 */
extern void MCL_FP_add(MCL_BIG x,MCL_BIG y,MCL_BIG z);
/**	@brief Modular subtraction of two MCL_BIGs in n-residue form, mod MCL_Modulus
 *
	@param x MCL_BIG number, on exit the modular difference = y-z mod MCL_Modulus
	@param y MCL_BIG number
	@param z MCL_BIG number
 */
extern void MCL_FP_sub(MCL_BIG x,MCL_BIG y,MCL_BIG z);
/**	@brief Modular division by 2 of a MCL_BIG in n-residue form, mod MCL_Modulus
 *
	@param x MCL_BIG number, on exit =y/2 mod MCL_Modulus
	@param y MCL_BIG number
 */
extern void MCL_FP_div2(MCL_BIG x,MCL_BIG y);
/**	@brief Fast Modular exponentiation of a MCL_BIG in n-residue form, to the power of a MCL_BIG, mod MCL_Modulus
 *
	@param x MCL_BIG number, on exit  = y^z mod MCL_Modulus
	@param y MCL_BIG number
	@param z Big number exponent
 */
extern void MCL_FP_pow(MCL_BIG x,MCL_BIG y,MCL_BIG z);
/**	@brief Fast Modular square root of a MCL_BIG in n-residue form, mod MCL_Modulus
 *
	@param x MCL_BIG number, on exit  = sqrt(y) mod MCL_Modulus
	@param y MCL_BIG number, the number whose square root is calculated

 */
extern void MCL_FP_sqrt(MCL_BIG x,MCL_BIG y);
/**	@brief Modular negation of a MCL_BIG in n-residue form, mod MCL_Modulus
 *
	@param x MCL_BIG number, on exit = -y mod MCL_Modulus
	@param y MCL_BIG number
 */
extern void MCL_FP_neg(MCL_BIG x,MCL_BIG y);
/**	@brief Outputs a MCL_BIG number that is in n-residue form to the console
 *
	Converts from n-residue form before output
	@param x a MCL_BIG number
 */
extern void MCL_FP_output(MCL_BIG x);
/**	@brief Outputs a MCL_BIG number that is in n-residue form to the console, in raw form
 *
	Converts from n-residue form before output
	@param x a MCL_BIG number
 */
extern void MCL_FP_rawoutput(MCL_BIG x);
/**	@brief Reduces possibly unreduced MCL_BIG mod MCL_Modulus
 *
	@param x MCL_BIG number, on exit reduced mod MCL_Modulus
 */
extern void MCL_FP_reduce(MCL_BIG x);
/**	@brief Tests for MCL_BIG a quadratic residue mod MCL_Modulus
 *
	@param x MCL_BIG number to be tested
	@return 1 if quadratic residue, else returns 0 if quadratic non-residue
 */
extern int MCL_FP_qr(MCL_BIG x);
/**	@brief Modular inverse of a MCL_BIG in n-residue form, mod MCL_Modulus
 *
	@param x MCL_BIG number, on exit = 1/y mod MCL_Modulus
	@param y MCL_BIG number
 */
extern void MCL_FP_inv(MCL_BIG x,MCL_BIG y);

#endif
