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

/* ARAcrypt X.509 Functions */

#include "mcl_x509.h"

// ASN.1 tags

#define ANY 0x00
#define SEQ 0x30
#define OID 0x06
#define INT 0x02
#define NUL 0x05
#define ZER 0x00
#define UTF 0x0C
#define UTC 0x17
#define LOG 0x01
#define BIT 0x03
#define OCT 0x04
#define STR 0x13
#define SET 0x31
#define IA5 0x16

// Supported Encryption Methods

#define ECC 1
#define RSA 2
#define ECC_H160 10
#define ECC_H256 11
#define ECC_H384 12
#define ECC_H512 13
#define RSA_H160 20
#define RSA_H256 21
#define RSA_H384 22
#define RSA_H512 23

// return xxxxxxxxxxxxxxxx | xxxx | xxxx
//        2048 | 2 | 3  -> 2048-bit RSA with SHA512

#define H160 1
#define H256 2
#define H384 3
#define H512 4

// Define some OIDs

// Elliptic Curve with SHA1
static char eccsha160[7]={0x2a,0x86,0x48,0xce,0x3d,0x04,0x01};
static mcl_octet ECCSHA160={7,sizeof(eccsha160),eccsha160};

// Elliptic Curve with SHA256
static char eccsha256[8]={0x2a,0x86,0x48,0xce,0x3d,0x04,0x03,0x02};
static mcl_octet ECCSHA256={8,sizeof(eccsha256),eccsha256};

// Elliptic Curve with SHA384
static char eccsha384[8]={0x2a,0x86,0x48,0xce,0x3d,0x04,0x03,0x03};
static mcl_octet ECCSHA384={8,sizeof(eccsha384),eccsha384};

// Elliptic Curve with SHA512
static char eccsha512[8]={0x2a,0x86,0x48,0xce,0x3d,0x04,0x03,0x04};
static mcl_octet ECCSHA512={8,sizeof(eccsha512),eccsha512};

// EC Public Key
static char ecpk[7]={0x2a,0x86,0x48,0xce,0x3d,0x02,0x01};
static mcl_octet ECPK={7,sizeof(ecpk),ecpk};

// NIST256 curve
static char prime256v1[8]={0x2a,0x86,0x48,0xce,0x3d,0x03,0x01,0x07};
static mcl_octet PRIME256V1={8,sizeof(prime256v1),prime256v1};

// NIST384 curve
static char secp384r1[5]={0x2B,0x81,0x04,0x00,0x22};
static mcl_octet SECP384R1={5,sizeof(secp384r1),secp384r1};

// NIST521 curve
static char secp521r1[5]={0x2B,0x81,0x04,0x00,0x23};
static mcl_octet SECP521R1={5,sizeof(secp521r1),secp521r1};

// RSA Public Key
static char rsapk[9]={0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x01};
static mcl_octet RSAPK={9,sizeof(rsapk),rsapk};

// RSA with SHA1
static char rsasha160[9]={0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x05};
static mcl_octet RSASHA160={9,sizeof(rsasha160),rsasha160};

// RSA with SHA256
static char rsasha256[9]={0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x0b};
static mcl_octet RSASHA256={9,sizeof(rsasha256),rsasha256};

// RSA with SHA384
static char rsasha384[9]={0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x0c}; 
static mcl_octet RSASHA384={9,sizeof(rsasha384),rsasha384};

// RSA with SHA512
static char rsasha512[9]={0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x0d}; 
static mcl_octet RSASHA512={9,sizeof(rsasha512),rsasha512};

/* Check expected TAG and return ASN.1 field length. If tag=0 skip check. */
static int getalen(int tag,char *b,int j)
{
	int len;

	if (tag!=0 && (unsigned char)b[j]!=tag) return -1; // not a valid tag
	j++;

	if ((unsigned char)b[j]==0x81)
	{
		j++;
		len=(unsigned char)b[j];
	}
	else if ((unsigned char)b[j]==0x82)
	{
		j++;
		len=256*b[j++];
		len+=(unsigned char)b[j];
	}
	else 
	{
		len=(unsigned char)b[j];
		if (len>127) return -1;
	}
	return len;
}

/* jump over length field */
static int skip(int len)
{
	if (len<128) return 2;
	if (len>=128 && len<256) return 3;
	return 4;
}

/* round length up to nearest 8-byte length */
static int bround(int len)
{
	if (len%8==0) return len;
	return len+(8-len%8);
	
}

//	Input signed cert as mcl_octet, and extract signature
//	Return 0 for failure, ECC for Elliptic Curve signature, RSA for RSA signature

pktype MCL_X509_extract_cert_sig(mcl_octet *sc,mcl_octet *sig)
{
	int i,j,k,fin,len,rlen,sj,ex;
	char soid[8];
	mcl_octet SOID={0,sizeof(soid),soid};
	pktype ret;

	ret.type=0; ret.hash=0;

	j=0;
		
	len=getalen(SEQ,sc->val,j);		// Check for expected SEQ clause, and get length
	if (len<0) return ret;			// if not a SEQ clause, there is a problem, exit
	j+=skip(len);					// skip over length to clause contents. Add len to skip clause

	if (len+j!=sc->len) return ret;

	len=getalen(SEQ,sc->val,j);
	if (len<0) return ret;
	j+=skip(len)+len; // jump over cert to signature OID

	len=getalen(SEQ,sc->val,j);
	if (len<0) return ret;
	j+=skip(len);

	sj=j+len; // Needed to jump over signature OID

// dive in to extract OID
	len=getalen(OID,sc->val,j);
	if (len<0) return ret;
	j+=skip(len);

	fin=j+len;
	SOID.len=len;
	for (i=0;j<fin;j++)
			SOID.val[i++]= sc->val[j];

	// check OID here..

	if (MCL_OCT_comp(&ECCSHA160,&SOID)) {ret.type=ECC; ret.hash=H160;} 
	if (MCL_OCT_comp(&ECCSHA256,&SOID)) {ret.type=ECC; ret.hash=H256;} 
	if (MCL_OCT_comp(&ECCSHA384,&SOID)) {ret.type=ECC; ret.hash=H384;}
	if (MCL_OCT_comp(&ECCSHA512,&SOID)) {ret.type=ECC; ret.hash=H512;} 
	if (MCL_OCT_comp(&RSASHA160,&SOID)) {ret.type=RSA; ret.hash=H160;}
	if (MCL_OCT_comp(&RSASHA256,&SOID)) {ret.type=RSA; ret.hash=H256;}
	if (MCL_OCT_comp(&RSASHA384,&SOID)) {ret.type=RSA; ret.hash=H384;}
	if (MCL_OCT_comp(&RSASHA512,&SOID)) {ret.type=RSA; ret.hash=H512;}

	if (ret.type==0) return ret; // unsupported type

	j=sj;  // jump out to signature

	len=getalen(BIT,sc->val,j);
	if (len<0) {ret.type=0; return ret;}
	j+=skip(len);
	j++; len--; // skip bit shift (hopefully 0!)

	if (ret.type==ECC)
	{ // signature in the form (r,s)
		len=getalen(SEQ,sc->val,j);
		if (len<0) {ret.type=0; return ret;}
		j+=skip(len);

	// pick up r part of signature
		len=getalen(INT,sc->val,j);
		if (len<0) {ret.type=0; return ret;}
		j+=skip(len);

		if (sc->val[j]==0)
		{ // skip leading zero
			j++;
			len--;
		}

		rlen=bround(len);
		ex=rlen-len;

		sig->len=2*rlen;

		i=0;
		for (k=0;k<ex;k++)
			sig->val[i++]=0;

		fin=j+len;
		for (;j<fin;j++)
				sig->val[i++]= sc->val[j];

	// pick up s part of signature
		len=getalen(INT,sc->val,j);
		if (len<0) {ret.type=0; return ret;}
		j+=skip(len);

		if (sc->val[j]==0)
		{ // skip leading zeros
			j++;
			len--;
		}
		rlen=bround(len);
		ex=rlen-len;

		for (k=0;k<ex;k++)
			sig->val[i++]=0;

		fin=j+len;
		for (;j<fin;j++)
				sig->val[i++]= sc->val[j];

	}
	if (ret.type==RSA)
	{
		rlen=bround(len);
		ex=rlen-len;

		sig->len=rlen;
		i=0;
		for (k=0;k<ex;k++)
			sig->val[i++]=0;

		fin=j+len;
		for (;j<fin;j++)
				sig->val[i++]= sc->val[j];

	}
	if (ret.hash==H256) ret.curve=MCL_NIST256;
	if (ret.hash==H384) ret.curve=MCL_NIST384;
	if (ret.hash==H512) ret.curve=MCL_NIST521;

	return ret;
}

// Extract certificate from signed cert
int MCL_X509_extract_cert(mcl_octet *sc,mcl_octet *cert)
{
	int i,j,fin,len,k;

	j=0;
	len=getalen(SEQ,sc->val,j);

	if (len<0) return 0;
	j+=skip(len);

	k=j;

	len=getalen(SEQ,sc->val,j);
	if (len<0) return 0;
	j+=skip(len);

	fin=j+len;
	cert->len=fin-k;
	for (i=k;i<fin;i++) cert->val[i-k]=sc->val[i];

	return 1;
}

// Extract Public Key from inside Certificate
pktype MCL_X509_extract_public_key(mcl_octet *c,mcl_octet *key)
{
	int i,j,fin,len,sj;
	char koid[8];
	mcl_octet KOID={0,sizeof(koid),koid};
	pktype ret;

	ret.type=ret.hash=0;

	j=0;

	len=getalen(SEQ,c->val,j);
	if (len<0) return ret;
	j+=skip(len);

	if (len+j!=c->len) return ret;

	len=getalen(0,c->val,j);
	if (len<0) return ret;
	j+=skip(len)+len; //jump over version clause

	len=getalen(INT,c->val,j);

	if (len>0) j+=skip(len)+len; // jump over serial number clause (if there is one)

	len=getalen(SEQ,c->val,j);
	if (len<0) return ret;
	j+=skip(len)+len;  // jump over signature algorithm

	len=getalen(SEQ,c->val,j);
	if (len<0) return ret;
	j+=skip(len)+len; // skip issuer

	len=getalen(SEQ,c->val,j);
	if (len<0) return ret;
	j+=skip(len)+len; // skip validity

	len=getalen(SEQ,c->val,j);
	if (len<0) return ret;
	j+=skip(len)+len; // skip subject

	len=getalen(SEQ,c->val,j);
	if (len<0) return ret;
	j+=skip(len); // 

	len=getalen(SEQ,c->val,j);
	if (len<0) return ret;
	j+=skip(len);

// ** Maybe dive in and check Public Key OIDs here?
// ecpublicKey & prime256v1, secp384r1 or secp521r1 for ECC
// rsapublicKey for RSA

	sj=j+len;

	len=getalen(OID,c->val,j);
	if (len<0) return ret;
	j+=skip(len);

	fin=j+len;
	KOID.len=len;
	for (i=0;j<fin;j++)
			KOID.val[i++]= c->val[j];

	ret.type=0;
	if (MCL_OCT_comp(&ECPK,&KOID)) ret.type=ECC;
	if (MCL_OCT_comp(&RSAPK,&KOID)) ret.type=RSA;

	if (ret.type==0) return ret;

	if (ret.type==ECC)
	{ // which elliptic curve?
		len=getalen(OID,c->val,j);
		if (len<0) {ret.type=0; return ret;}
		j+=skip(len);

		fin=j+len;
		KOID.len=len;
		for (i=0;j<fin;j++)
				KOID.val[i++]= c->val[j];

		ret.curve=0;
		if (MCL_OCT_comp(&PRIME256V1,&KOID)) ret.curve=MCL_NIST256;
		if (MCL_OCT_comp(&SECP384R1,&KOID)) ret.curve=MCL_NIST384;
		if (MCL_OCT_comp(&SECP521R1,&KOID)) ret.curve=MCL_NIST521;
	}

	j=sj; // skip to actual Public Key

	len=getalen(BIT,c->val,j);
	if (len<0) {ret.type=0; return ret;}
	j+=skip(len); // 
	j++; len--; // skip bit shift (hopefully 0!)

// extract key
	if (ret.type==ECC)
	{
		key->len=len;
		fin=j+len;
		for (i=0;j<fin;j++)
			key->val[i++]= c->val[j];

	}
	if (ret.type==RSA)
	{ // Key is (modulus,exponent) - assume exponent is 65537
		len=getalen(SEQ,c->val,j);
		if (len<0) {ret.type=0; return ret;}
		j+=skip(len); // 

		len=getalen(INT,c->val,j); // get modulus
		if (len<0) {ret.type=0; return ret;}
		j+=skip(len); // 
		if (c->val[j]==0)
		{
			j++; len--; // remove leading zero
		}

		key->len=len;
		fin=j+len;
		for (i=0;j<fin;j++)
			key->val[i++]= c->val[j];

	}
	return ret;
}

// Find pointer to main sections of cert, before extracting individual field 
// Find index to issuer in cert
int MCL_X509_find_issuer(mcl_octet *c)
{
	int j,len;
	j=0;
	len=getalen(SEQ,c->val,j);
	if (len<0) return 0;
	j+=skip(len);

	if (len+j!=c->len) return 0;

	len=getalen(0,c->val,j);
	if (len<0) return 0;
	j+=skip(len)+len; //jump over version clause

	len=getalen(INT,c->val,j);

	if (len>0) j+=skip(len)+len; // jump over serial number clause (if there is one)

	len=getalen(SEQ,c->val,j);
	if (len<0) return 0;
	j+=skip(len)+len;  // jump over signature algorithm

	return j;
}

// Find index to validity period
int MCL_X509_find_validity(mcl_octet *c)
{
	int j,len;
	j=MCL_X509_find_issuer(c);

	len=getalen(SEQ,c->val,j);
	if (len<0) return 0;
	j+=skip(len)+len; // skip issuer

	return j;
}

// Find index to subject in cert
int MCL_X509_find_subject(mcl_octet *c)
{
	int j,len;
	j=MCL_X509_find_validity(c);

	len=getalen(SEQ,c->val,j);
	if (len<0) return 0;
	j+=skip(len)+len; // skip validity

	return j;
}

// NOTE: When extracting cert information, we actually return just an index to the data inside the cert, and maybe its length
// So no memory is assigned to store cert info. It is the callers responsibility to allocate such memory if required, and copy
// cert information into it.

// Find entity property indicated by SOID, given start of issuer or subject field. Return index in cert, flen=length of field

int MCL_X509_find_entity_property(mcl_octet *c,mcl_octet *SOID,int start,int *flen)
{
	int i,j,k,fin,len,tlen;
	char foid[20];
	mcl_octet FOID={0,sizeof(foid),foid};

	j=start;
	
	tlen=getalen(SEQ,c->val,j);
	if (tlen<0) return 0;
	j+=skip(tlen);

	for (k=j;j<k+tlen;)
	{ // search for Owner OID
		len=getalen(SET,c->val,j);
		if (len<0) return 0;
		j+=skip(len);
		len=getalen(SEQ,c->val,j);
		if (len<0) return 0;
		j+=skip(len);
		len=getalen(OID,c->val,j);
		if (len<0) return 0;
		j+=skip(len);
		fin=j+len;  // extract OID
		FOID.len=len;
		for (i=0;j<fin;j++)
			FOID.val[i++]= c->val[j];
		len=getalen(ANY,c->val,j);  // get text, could be any type
		if (len<0) return 0;

		j+=skip(len);
		if (MCL_OCT_comp(&FOID,SOID))
		{ // if its the right one return
			*flen=len;
			return j;
		}
		j+=len;  // skip over it
	}

	return 0;
}

// Find start date of certificate validity period
int MCL_X509_find_start_date(mcl_octet *c,int start)
{
	int j,len;
	j=start;

	len=getalen(SEQ,c->val,j);
	if (len<0) return 0;
	j+=skip(len);

	len=getalen(UTC,c->val,j);
	if (len<0) return 0;
	j+=skip(len);
	return j;
}

// Find expiry date of certificate validity period
int MCL_X509_find_expiry_date(mcl_octet *c,int start)
{
	int j,len;
	j=start;

	len=getalen(SEQ,c->val,j);
	if (len<0) return 0;
	j+=skip(len);

	len=getalen(UTC,c->val,j);
	if (len<0) return 0;
	j+=skip(len)+len;

	len=getalen(UTC,c->val,j);
	if (len<0) return 0;
	j+=skip(len);

	return j;
}
