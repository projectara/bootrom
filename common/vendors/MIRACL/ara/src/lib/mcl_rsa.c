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


/* RSA Functions - see main program below */

#include "mcl_rsa.h"

#define ROUNDUP(a,b) ((a)-1)/(b)+1

/* general purpose hash function w=hash(p|n|x|y) */
static int hashit(int sha,mcl_octet *p,int n,mcl_octet *w)
{
    int i,c[4],hlen;
    mcl_hash256 sha256;
	mcl_hash512 sha512;
    char hh[64];

	switch (sha)
	{
	case MCL_SHA1 :
		MCL_HASH160_init(&sha256); break;
	case MCL_SHA256:
		MCL_HASH256_init(&sha256); break;
	case MCL_SHA384:
		MCL_HASH384_init(&sha512); break;
	case MCL_SHA512:
		MCL_HASH512_init(&sha512); break;
	}
    
	hlen=sha;

    if (p!=NULL) for (i=0;i<p->len;i++)
	{	
		switch(sha)
		{
		case MCL_SHA1:
			MCL_HASH160_process(&sha256,p->val[i]); break;
		case MCL_SHA256:
			MCL_HASH256_process(&sha256,p->val[i]); break;
		case MCL_SHA384:
			MCL_HASH384_process(&sha512,p->val[i]); break;
		case MCL_SHA512:
			MCL_HASH512_process(&sha512,p->val[i]); break;
		}
	}
	if (n>=0)
    {
        c[0]=(n>>24)&0xff;
        c[1]=(n>>16)&0xff;
        c[2]=(n>>8)&0xff;
        c[3]=(n)&0xff;
		for (i=0;i<4;i++)
		{
			switch(sha)
			{
			case MCL_SHA1:
				MCL_HASH160_process(&sha256,c[i]); break;
			case MCL_SHA256:
				MCL_HASH256_process(&sha256,c[i]); break;
			case MCL_SHA384:
				MCL_HASH384_process(&sha512,c[i]); break;
			case MCL_SHA512:
				MCL_HASH512_process(&sha512,c[i]); break;
			}
		}
    }
	  
	switch (sha)
	{
	case MCL_SHA1:
		MCL_HASH160_hash(&sha256,hh); break;
	case MCL_SHA256:
		MCL_HASH256_hash(&sha256,hh); break;
	case MCL_SHA384:
		MCL_HASH384_hash(&sha512,hh); break;
	case MCL_SHA512:
		MCL_HASH512_hash(&sha512,hh); break;
	}
   
    MCL_OCT_empty(w);
    MCL_OCT_jbytes(w,hh,hlen);
    for (i=0;i<hlen;i++) hh[i]=0;

	return hlen;
}

/* Initialise a Cryptographically Strong Random Number Generator from 
   an mcl_octet of raw random data */

void MCL_RSA_CREATE_CSPRNG(csprng *RNG,mcl_octet *RAW)
{
    MCL_RAND_seed(RNG,RAW->len,RAW->val);
}

void MCL_RSA_KILL_CSPRNG(csprng *RNG)
{
    MCL_RAND_clean(RNG);
}

/* generate an RSA key pair */

void MCL_RSA_KEY_PAIR(csprng *RNG,sign32 e,MCL_rsa_private_key *PRIV,MCL_rsa_public_key *PUB)
{ /* IEEE1363 A16.11/A16.12 more or less */

    mcl_chunk t[MCL_HFLEN][MCL_BS],p1[MCL_HFLEN][MCL_BS],q1[MCL_HFLEN][MCL_BS];
    
	for (;;)
	{

		MCL_FF_random(PRIV->p,RNG,MCL_HFLEN);
		while (MCL_FF_lastbits(PRIV->p,2)!=3) MCL_FF_inc(PRIV->p,1,MCL_HFLEN);
		while (!MCL_FF_prime(PRIV->p,RNG,MCL_HFLEN))
			MCL_FF_inc(PRIV->p,4,MCL_HFLEN);
		MCL_FF_copy(p1,PRIV->p,MCL_HFLEN);
		MCL_FF_dec(p1,1,MCL_HFLEN);

		if (MCL_FF_cfactor(p1,e,MCL_HFLEN)) continue;
		break;
	}

	for (;;)
	{
		MCL_FF_random(PRIV->q,RNG,MCL_HFLEN);
		while (MCL_FF_lastbits(PRIV->q,2)!=3) MCL_FF_inc(PRIV->q,1,MCL_HFLEN);
		while (!MCL_FF_prime(PRIV->q,RNG,MCL_HFLEN))
			MCL_FF_inc(PRIV->q,4,MCL_HFLEN);

		MCL_FF_copy(q1,PRIV->q,MCL_HFLEN);	
		MCL_FF_dec(q1,1,MCL_HFLEN);
		if (MCL_FF_cfactor(q1,e,MCL_HFLEN)) continue;

		break;
	}

	MCL_FF_mul(PUB->n,PRIV->p,PRIV->q,MCL_HFLEN);
	PUB->e=e;

	MCL_FF_copy(t,p1,MCL_HFLEN);
	MCL_FF_shr(t,MCL_HFLEN);
	MCL_FF_init(PRIV->dp,e,MCL_HFLEN);
	MCL_FF_invmodp(PRIV->dp,PRIV->dp,t,MCL_HFLEN);
	if (MCL_FF_parity(PRIV->dp)==0) MCL_FF_add(PRIV->dp,PRIV->dp,t,MCL_HFLEN);
	MCL_FF_norm(PRIV->dp,MCL_HFLEN);

	MCL_FF_copy(t,q1,MCL_HFLEN);
	MCL_FF_shr(t,MCL_HFLEN);
	MCL_FF_init(PRIV->dq,e,MCL_HFLEN);
	MCL_FF_invmodp(PRIV->dq,PRIV->dq,t,MCL_HFLEN);
	if (MCL_FF_parity(PRIV->dq)==0) MCL_FF_add(PRIV->dq,PRIV->dq,t,MCL_HFLEN);
	MCL_FF_norm(PRIV->dq,MCL_HFLEN);

	MCL_FF_invmodp(PRIV->c,PRIV->p,PRIV->q,MCL_HFLEN);

	return;
}

/* Mask Generation Function */

static void MGF1(int sha,mcl_octet *z,int olen,mcl_octet *mask)
{
	char h[64];
    mcl_octet H={0,sizeof(h),h};
	int hlen=sha;
    int counter,cthreshold;

    MCL_OCT_empty(mask);

    cthreshold=ROUNDUP(olen,hlen);

    for (counter=0;counter<cthreshold;counter++)
    {
        hashit(sha,z,counter,&H);
        if (mask->len+hlen>olen) MCL_OCT_jbytes(mask,H.val,olen%hlen);
        else                     MCL_OCT_jmcl_octet(mask,&H);
    }
    MCL_OCT_clear(&H);
}

/* SHA256 identifier string */
const char SHA256ID[]={0x30,0x31,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x01,0x05,0x00,0x04,0x20};
const char SHA384ID[]={0x30,0x41,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x02,0x05,0x00,0x04,0x30};
const char SHA512ID[]={0x30,0x51,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x03,0x05,0x00,0x04,0x40};

/* PKCS 1.5 padding of a message to be signed */

int MCL_PKCS15(int sha,mcl_octet *m,mcl_octet *w)
{
	int olen=MCL_FF_BITS/8;
	int hlen=sha;
	int idlen=19;
	char h[64];
	mcl_octet H={0,sizeof(h),h}; 

    if (olen<idlen+hlen+10) return 0;
	hashit(sha,m,-1,&H);

	MCL_OCT_empty(w);
	MCL_OCT_jbyte(w,0x00,1);
    MCL_OCT_jbyte(w,0x01,1);
    MCL_OCT_jbyte(w,0xff,olen-idlen-hlen-3);
    MCL_OCT_jbyte(w,0x00,1);

    if (hlen==32) MCL_OCT_jbytes(w,(char *)SHA256ID,idlen);
    if (hlen==48) MCL_OCT_jbytes(w,(char *)SHA384ID,idlen);
    if (hlen==64) MCL_OCT_jbytes(w,(char *)SHA512ID,idlen);

	MCL_OCT_jmcl_octet(w,&H);

	return 1;
}

/* OAEP Message Encoding for Encryption */

int MCL_OAEP_ENCODE(int sha,mcl_octet *m,csprng *RNG,mcl_octet *p,mcl_octet *f)
{ 
    int slen,olen=MCL_RFS-1;
    int mlen=m->len;
    int hlen,seedlen;
    char dbmask[MCL_RFS],seed[64];
	mcl_octet DBMASK={0,sizeof(dbmask),dbmask};   
	mcl_octet SEED={0,sizeof(seed),seed};

    hlen=seedlen=sha;
    if (mlen>olen-hlen-seedlen-1) return 0;
    if (m==f) return 0;  /* must be distinct mcl_octets */ 

    hashit(sha,p,-1,f);

    slen=olen-mlen-hlen-seedlen-1;      

    MCL_OCT_jbyte(f,0,slen);
    MCL_OCT_jbyte(f,0x1,1);
    MCL_OCT_jmcl_octet(f,m);

    MCL_OCT_rand(&SEED,RNG,seedlen);

    MGF1(sha,&SEED,olen-seedlen,&DBMASK);

    MCL_OCT_xor(&DBMASK,f);
    MGF1(sha,&DBMASK,seedlen,f);

    MCL_OCT_xor(f,&SEED);

    MCL_OCT_jmcl_octet(f,&DBMASK);

	MCL_OCT_pad(f,MCL_RFS);
    MCL_OCT_clear(&SEED);
    MCL_OCT_clear(&DBMASK);

    return 1;
}

/* OAEP Message Decoding for Decryption */

int MCL_OAEP_DECODE(int sha,mcl_octet *p,mcl_octet *f)
{
    int comp,x,t;
    int i,k,olen=MCL_RFS-1;
    int hlen,seedlen;
    char dbmask[MCL_RFS],seed[64],chash[64];
	mcl_octet DBMASK={0,sizeof(dbmask),dbmask};   
	mcl_octet SEED={0,sizeof(seed),seed};
	mcl_octet CMCL_HASH={0,sizeof(chash),chash};

    seedlen=hlen=sha;
    if (olen<seedlen+hlen+1) return 0;
    if (!MCL_OCT_pad(f,olen+1)) return 0;
	hashit(sha,p,-1,&CMCL_HASH);

    x=f->val[0];
    for (i=seedlen;i<olen;i++)
        DBMASK.val[i-seedlen]=f->val[i+1]; 
    DBMASK.len=olen-seedlen;

    MGF1(sha,&DBMASK,seedlen,&SEED);
    for (i=0;i<seedlen;i++) SEED.val[i]^=f->val[i+1];
    MGF1(sha,&SEED,olen-seedlen,f);
    MCL_OCT_xor(&DBMASK,f);

    comp=MCL_OCT_ncomp(&CMCL_HASH,&DBMASK,hlen);

    MCL_OCT_shl(&DBMASK,hlen);

    MCL_OCT_clear(&SEED);
    MCL_OCT_clear(&CMCL_HASH);

    for (k=0;;k++)
    {
        if (k>=DBMASK.len)
        {
            MCL_OCT_clear(&DBMASK);
            return 0;
        }
        if (DBMASK.val[k]!=0) break;
    }

    t=DBMASK.val[k];
    if (!comp || x!=0 || t!=0x01) 
    {
        MCL_OCT_clear(&DBMASK);
        return 0;
    }

    MCL_OCT_shl(&DBMASK,k+1);
    MCL_OCT_copy(f,&DBMASK);
    MCL_OCT_clear(&DBMASK);

    return 1;
}

/* destroy the Private Key structure */
void MCL_RSA_PRIVATE_KEY_KILL(MCL_rsa_private_key *PRIV)
{
    MCL_FF_zero(PRIV->p,MCL_HFLEN);
	MCL_FF_zero(PRIV->q,MCL_HFLEN);
	MCL_FF_zero(PRIV->dp,MCL_HFLEN);
	MCL_FF_zero(PRIV->dq,MCL_HFLEN);
	MCL_FF_zero(PRIV->c,MCL_HFLEN);
}

/* RSA encryption with the public key */
void MCL_RSA_ENCRYPT(MCL_rsa_public_key *PUB,mcl_octet *F,mcl_octet *G)
{
	mcl_chunk f[MCL_FFLEN][MCL_BS];
	MCL_FF_fromOctet(f,F,MCL_FFLEN);

    MCL_FF_power(f,f,PUB->e,PUB->n,MCL_FFLEN);

	MCL_FF_toOctet(G,f,MCL_FFLEN);
}

/* RSA decryption with the private key */
void MCL_RSA_DECRYPT(MCL_rsa_private_key *PRIV,mcl_octet *G,mcl_octet *F)
{
	mcl_chunk g[MCL_FFLEN][MCL_BS],t[MCL_FFLEN][MCL_BS],jp[MCL_HFLEN][MCL_BS],jq[MCL_HFLEN][MCL_BS];

	MCL_FF_fromOctet(g,G,MCL_FFLEN);	
	
	MCL_FF_dmod(jp,g,PRIV->p,MCL_HFLEN);
	MCL_FF_dmod(jq,g,PRIV->q,MCL_HFLEN);

	MCL_FF_skpow(jp,jp,PRIV->dp,PRIV->p,MCL_HFLEN);
	MCL_FF_skpow(jq,jq,PRIV->dq,PRIV->q,MCL_HFLEN);


	MCL_FF_zero(g,MCL_FFLEN);
	MCL_FF_copy(g,jp,MCL_HFLEN);
	MCL_FF_mod(jp,PRIV->q,MCL_HFLEN);
	if (MCL_FF_comp(jp,jq,MCL_HFLEN)>0)
		MCL_FF_add(jq,jq,PRIV->q,MCL_HFLEN);
	MCL_FF_sub(jq,jq,jp,MCL_HFLEN);
	MCL_FF_norm(jq,MCL_HFLEN);

	MCL_FF_mul(t,PRIV->c,jq,MCL_HFLEN);
	MCL_FF_dmod(jq,t,PRIV->q,MCL_HFLEN);

	MCL_FF_mul(t,jq,PRIV->p,MCL_HFLEN);
	MCL_FF_add(g,t,g,MCL_FFLEN);
	MCL_FF_norm(g,MCL_FFLEN);
 
	MCL_FF_toOctet(F,g,MCL_FFLEN);

	return;
}

