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

/* ARA MCL_ECP header file */
/* Designed for AES128 to AES256 security, 256 to 521 bit elliptic curves */

/**
 * @file mcl_ecp.h
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

#ifndef MCL_ECP_H
#define MCL_ECP_H

/* Curve Params - see rom.c */
extern const int MCL_CURVE_A; /**< Elliptic curve A parameter */
extern const mcl_chunk MCL_CURVE_B[]; /**< Elliptic curve B parameter */
extern const mcl_chunk MCL_CURVE_Order[]; /**< Elliptic curve group order */

/* Generator point on G1 */
extern const mcl_chunk MCL_CURVE_Gx[]; /**< x-coordinate of generator point in group G1  */
extern const mcl_chunk MCL_CURVE_Gy[]; /**< y-coordinate of generator point in group G1  */

/**
	@brief MCL_ECP structure - Elliptic Curve Point over base field
*/

typedef struct {
#if MCL_CURVETYPE!=MCL_EDWARDS
int inf; /**< Infinity Flag - not needed for Edwards representation */
#endif
mcl_chunk x[MCL_BS];   /**< x-coordinate of point */
#if MCL_CURVETYPE!=MCL_MONTGOMERY
mcl_chunk y[MCL_BS];  /**< y-coordinate of point. Not needed for Montgomery representation */
#endif
mcl_chunk z[MCL_BS]; /**< z-coordinate of point */
} MCL_ECP;


#include "mcl_oct.h"

/* MCL_ECP E(Fp) prototypes */
/**	@brief Tests for MCL_ECP point equal to infinity
 *
	@param P MCL_ECP point to be tested
	@return 1 if infinity, else returns 0
 */
extern int MCL_ECP_isinf(MCL_ECP *P);
/**	@brief Tests for equality of two MCL_ECPs
 *
	@param P MCL_ECP instance to be compared
	@param Q MCL_ECP instance to be compared
	@return 1 if P=Q, else returns 0
 */
extern int MCL_ECP_equals(MCL_ECP *P,MCL_ECP *Q);
/**	@brief Copy MCL_ECP point to another MCL_ECP point
 *
	@param P MCL_ECP instance, on exit = Q
	@param Q MCL_ECP instance to be copied
 */
extern void MCL_ECP_copy(MCL_ECP *P,MCL_ECP *Q);
/**	@brief Negation of an MCL_ECP point
 *
	@param P MCL_ECP instance, on exit = -P 
 */
extern void MCL_ECP_neg(MCL_ECP *P);
/**	@brief Set MCL_ECP to point-at-infinity
 *
	@param P MCL_ECP instance to be set to infinity
 */
extern void MCL_ECP_inf(MCL_ECP *P);
/**	@brief Calculate Right Hand Side of curve equation y^2=f(x)
 *
	Function f(x) depends on form of elliptic curve, Weierstrass, Edwards or Montgomery.
	Used internally.
	@param r MCL_BIG n-residue value of f(x)
	@param x MCL_BIG n-residue x
 */
extern void MCL_ECP_rhs(MCL_BIG r,MCL_BIG x);
/**	@brief Set MCL_ECP to point(x,y) given just x and sign of y
 *
	Point P set to infinity if no such point on the curve. If x is on the curve then y is calculated from the curve equation.
	The correct y value (plus or minus) is selected given its sign s.
	@param P MCL_ECP instance to be set (x,[y])
	@param x MCL_BIG x coordinate of point
	@param s an integer representing the "sign" of y, in fact its least significant bit.
 */
extern int MCL_ECP_setx(MCL_ECP *P,MCL_BIG x,int s);

#if MCL_CURVETYPE==MCL_MONTGOMERY
/**	@brief Set MCL_ECP to point(x,[y]) given x
 *
	Point P set to infinity if no such point on the curve. Note that y coordinate is not needed.
	@param P MCL_ECP instance to be set (x,[y])
	@param x MCL_BIG x coordinate of point
	@return 1 if point exists, else 0
 */
extern int MCL_ECP_set(MCL_ECP *P,MCL_BIG x);
/**	@brief Extract x coordinate of an MCL_ECP point P
 *
	@param x MCL_BIG on exit = x coordinate of point
	@param P MCL_ECP instance (x,[y])
	@return -1 if P is point-at-infinity, else 0
 */
extern int MCL_ECP_get(MCL_BIG x,MCL_ECP *P);
/**	@brief Adds MCL_ECP instance Q to MCL_ECP instance P, given difference D=P-Q
 *
	Differential addition of points on a Montgomery curve
	@param P MCL_ECP instance, on exit =P+Q
	@param Q MCL_ECP instance to be added to P
	@param D Difference between P and Q
 */
extern void MCL_ECP_add(MCL_ECP *P,MCL_ECP *Q,MCL_ECP *D);
#else
/**	@brief Set MCL_ECP to point(x,y) given x and y
 *
	Point P set to infinity if no such point on the curve. 
	@param P MCL_ECP instance to be set (x,y)
	@param x MCL_BIG x coordinate of point
	@param y MCL_BIG y coordinate of point
	@return 1 if point exists, else 0
 */
extern int MCL_ECP_set(MCL_ECP *P,MCL_BIG x,MCL_BIG y);
/**	@brief Extract x and y coordinates of an MCL_ECP point P
 *
	If x=y, returns only x
	@param x MCL_BIG on exit = x coordinate of point
	@param y MCL_BIG on exit = y coordinate of point (unless x=y)
	@param P MCL_ECP instance (x,y)
	@return sign of y, or -1 if P is point-at-infinity
 */
extern int MCL_ECP_get(MCL_BIG x,MCL_BIG y,MCL_ECP *P);
/**	@brief Adds MCL_ECP instance Q to MCL_ECP instance P
 *
	@param P MCL_ECP instance, on exit =P+Q
	@param Q MCL_ECP instance to be added to P
 */
extern void MCL_ECP_add(MCL_ECP *P,MCL_ECP *Q);
/**	@brief Subtracts MCL_ECP instance Q from MCL_ECP instance P
 *
	@param P MCL_ECP instance, on exit =P-Q
	@param Q MCL_ECP instance to be subtracted from P
 */
extern void MCL_ECP_sub(MCL_ECP *P,MCL_ECP *Q);
#endif
/**	@brief Converts an MCL_ECP point from Projective (x,y,z) coordinates to affine (x,y) coordinates
 *
	@param P MCL_ECP instance to be converted to affine form
 */
extern void MCL_ECP_affine(MCL_ECP *P);
/**	@brief Formats and outputs an MCL_ECP point to the console, in projective coordinates
 *
	@param P MCL_ECP instance to be printed
 */
extern void MCL_ECP_outputxyz(MCL_ECP *P);
/**	@brief Formats and outputs an MCL_ECP point to the console, converted to affine coordinates
 *
	@param P MCL_ECP instance to be printed
 */
extern void MCL_ECP_output(MCL_ECP * P);
/**	@brief Formats and outputs an MCL_ECP point to an mcl_octet string
 *
	The mcl_octet string is created in the standard form 04|x|y, except for Montgomery curve in which case it is 06|x
	Here x (and y) are the x and y coordinates in big-endian base 256 form.
	@param S output mcl_octet string
	@param P MCL_ECP instance to be converted to an mcl_octet string
 */
extern void MCL_ECP_toOctet(mcl_octet *S,MCL_ECP *P);
/**	@brief Creates an MCL_ECP point from an mcl_octet string
 *
	The mcl_octet string is in the standard form 0x04|x|y, except for Montgomery curve in which case it is 0x06|x
	Here x (and y) are the x and y coordinates in left justified big-endian base 256 form.
	@param P MCL_ECP instance to be created from the mcl_octet string
	@param S input mcl_octet string
	return 1 if mcl_octet string corresponds to a point on the curve, else 0
 */
extern int MCL_ECP_fromOctet(MCL_ECP *P,mcl_octet *S);
/**	@brief Doubles an MCL_ECP instance P
 *
	@param P MCL_ECP instance, on exit =2*P
 */
extern void MCL_ECP_dbl(MCL_ECP *P);
/**	@brief Multiplies an MCL_ECP instance P by a small integer, side-channel resistant
 *
	@param P MCL_ECP instance, on exit =i*P
	@param i small integer multiplier
	@param b maximum number of bits in multiplier
 */
extern void MCL_ECP_pinmul(MCL_ECP *P,int i,int b);
/**	@brief Multiplies an MCL_ECP instance P by a MCL_BIG, side-channel resistant
 *
	Uses Montgomery ladder for Montgomery curves, otherwise fixed sized windows.
	@param P MCL_ECP instance, on exit =b*P
	@param b MCL_BIG number multiplier

 */
extern void MCL_ECP_mul(MCL_ECP *P,MCL_BIG b);
/**	@brief Calculates double multiplication P=e*P+f*Q, side-channel resistant
 *
	@param P MCL_ECP instance, on exit =e*P+f*Q
	@param Q MCL_ECP instance
	@param e MCL_BIG number multiplier
	@param f MCL_BIG number multiplier
 */
extern void MCL_ECP_mul2(MCL_ECP *P,MCL_ECP *Q,MCL_BIG e,MCL_BIG f);

#endif
