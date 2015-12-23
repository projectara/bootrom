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

/* ARA x509 header file */

/**
 * @file x509.h
 * @author Mike Scott and Kealan McCusker
 * @date 19th May 2015
 * @brief MCL_X509 function Header File
 *
 * defines structures
 * declares functions
 * 
 */

#ifndef MCL_X509_H
#define MCL_X509_H

#include "mcl_arch.h"
#include "mcl_oct.h"

typedef struct {
int type;
int hash;
int curve;
} pktype;

/* X.509 functions */
/** @brief Extract certificate signature
 *
	@param c an X.509 certificate
	@param s the extracted signature
	@return 0 on failure, or indicator of signature type (ECC or RSA)

*/
extern pktype MCL_X509_extract_cert_sig(mcl_octet *c,mcl_octet *s);
/** @brief
 *
	@param sc a signed certificate
	@param c the extracted certificate
	@return 0 on failure
*/
extern int MCL_X509_extract_cert(mcl_octet *sc,mcl_octet *c);
/** @brief
 *
	@param c an X.509 certificate
	@param k the extracted key
	@return 0 on failure, or indicator of public key type (ECC or RSA)
*/
extern pktype MCL_X509_extract_public_key(mcl_octet *c,mcl_octet *k);
/** @brief
 *
	@param c an X.509 certificate
	@return 0 on failure, or pointer to issuer field in cert
*/
extern int MCL_X509_find_issuer(mcl_octet *c);
/** @brief
 *
	@param c an X.509 certificate
	@return 0 on failure, or pointer to validity field in cert
*/
extern int MCL_X509_find_validity(mcl_octet *c);
/** @brief
 *
	@param c an X.509 certificate
	@return 0 on failure, or pointer to subject field in cert
*/
extern int MCL_X509_find_subject(mcl_octet *c);
/** @brief
 *
	@param c an X.509 certificate
	@param S is OID of property we are looking for
	@param s is a pointer to the section of interest in the cert
	@param f is pointer to the length of the property
	@return 0 on failure, or pointer to the property
*/
extern int MCL_X509_find_entity_property(mcl_octet *c,mcl_octet *S,int s,int *f);
/** @brief
 *
	@param c an X.509 certificate
	@param s is a pointer to the start of the validity field
	@return 0 on failure, or pointer to the start date
*/
extern int MCL_X509_find_start_date(mcl_octet *c,int s);
/** @brief
 *
	@param c an X.509 certificate
	@param s is a pointer to the start of the validity field
	@return 0 on failure, or pointer to the expiry date
*/
extern int MCL_X509_find_expiry_date(mcl_octet *c,int s);


#endif
