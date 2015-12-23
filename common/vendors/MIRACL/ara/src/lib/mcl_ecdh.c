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


/* ECDH/ECIES/ECDSA Functions - see main program below */

#include "mcl_ecdh.h"

#define ROUNDUP(a,b) ((a)-1)/(b)+1

/* general purpose hash function w=hash(p|n|x|y) */
static void hashit(int sha,mcl_octet *p,int n,mcl_octet *x,mcl_octet *y,mcl_octet *w)
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
	if (n>0)
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
    if (x!=NULL) for (i=0;i<x->len;i++)
	{
		switch(sha)
		{
		case MCL_SHA1:
			MCL_HASH160_process(&sha256,x->val[i]); break;
		case MCL_SHA256:
			MCL_HASH256_process(&sha256,x->val[i]); break;
		case MCL_SHA384:
			MCL_HASH384_process(&sha512,x->val[i]); break;
		case MCL_SHA512:
			MCL_HASH512_process(&sha512,x->val[i]); break;
		}
	}
    if (y!=NULL) for (i=0;i<y->len;i++)
	{
		switch(sha)
		{
		case MCL_SHA1:
			MCL_HASH160_process(&sha256,y->val[i]); break;
		case MCL_SHA256:
			MCL_HASH256_process(&sha256,y->val[i]); break;
		case MCL_SHA384:
			MCL_HASH384_process(&sha512,y->val[i]); break;
		case MCL_SHA512:
			MCL_HASH512_process(&sha512,y->val[i]); break;
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
}

/* Hash mcl_octet p to mcl_octet w */
void MCL_HASH(int sha,mcl_octet *p,mcl_octet *w)
{
	hashit(sha,p,-1,NULL,NULL,w);
}

/* Initialise a Cryptographically Strong Random Number Generator from 
   an mcl_octet of raw random data */
void MCL_CREATE_CSPRNG(csprng *RNG,mcl_octet *RAW)
{
    MCL_RAND_seed(RNG,RAW->len,RAW->val);
}

void MCL_KILL_CSPRNG(csprng *RNG)
{
    MCL_RAND_clean(RNG);
}

/* Calculate MCL_HMAC of m using key k. MCL_HMAC is tag of length olen */
int MCL_HMAC(int sha,mcl_octet *m,mcl_octet *k,int olen,mcl_octet *tag)
{
/* Input is from an mcl_octet m        *
 * olen is requested output length in bytes. k is the key  *
 * The output is the calculated tag */
    int hlen,b;
	char h[64],k0[128];
    mcl_octet H={0,sizeof(h),h};
	mcl_octet K0={0,sizeof(k0),k0};

    hlen=sha; b=64;
	if (hlen>32) b=128;

    if (olen<4 /*|| olen>hlen+2*/) return 0;  

    if (k->len > b) hashit(sha,k,-1,NULL,NULL,&K0);
    else            MCL_OCT_copy(&K0,k);

    MCL_OCT_jbyte(&K0,0,b-K0.len);

    MCL_OCT_xorbyte(&K0,0x36);

    hashit(sha,&K0,-1,m,NULL,&H);

    MCL_OCT_xorbyte(&K0,0x6a);   /* 0x6a = 0x36 ^ 0x5c */
    hashit(sha,&K0,-1,&H,NULL,&H);
    MCL_OCT_empty(tag);

	if (olen>sha) olen=sha;
    MCL_OCT_jbytes(tag,H.val,olen);

    return 1;
}

/* Key Derivation Functions */
/* Input mcl_octet z */
/* Output key of length olen */
/*
void KDF1(mcl_octet *z,int olen,mcl_octet *key)
{
    char h[HLEN_ECC];
	mcl_octet H={0,sizeof(h),h};
    int counter,cthreshold;
    int hlen=32;
  
    MCL_OCT_empty(key);

    cthreshold=ROUNDUP(olen,hlen);

    for (counter=0;counter<cthreshold;counter++)
    {
        hashit(z,counter,NULL,NULL,&H);
        if (key->len+hlen>olen) MCL_OCT_jbytes(key,H.val,olen%hlen);
        else                    MCL_OCT_jmcl_octet(key,&H);
    }
}
*/
void MCL_KDF2(int sha,mcl_octet *z,mcl_octet *p,int olen,mcl_octet *key)
{
/* NOTE: the parameter olen is the length of the output k in bytes */
    char h[64];
	mcl_octet H={0,sizeof(h),h};
    int counter,cthreshold;
    int hlen=sha;
    
    MCL_OCT_empty(key);

    cthreshold=ROUNDUP(olen,hlen);

    for (counter=1;counter<=cthreshold;counter++)
    {
        hashit(sha,z,counter,p,NULL,&H);
        if (key->len+hlen>olen)  MCL_OCT_jbytes(key,H.val,olen%hlen);
        else                     MCL_OCT_jmcl_octet(key,&H);
    }
}

/* Password based Key Derivation Function */
/* Input password p, salt s, and repeat count */
/* Output key of length olen */
void MCL_PBKDF2(int sha,mcl_octet *p,mcl_octet *s,int rep,int olen,mcl_octet *key)
{
	int i,j,len,d=ROUNDUP(olen,sha);
	char f[MCL_EFS],u[MCL_EFS];
	mcl_octet F={0,sizeof(f),f};
	mcl_octet U={0,sizeof(u),u};
	MCL_OCT_empty(key);
	for (i=1;i<=d;i++)
	{
		len=s->len;
		MCL_OCT_jint(s,i,4);
		MCL_HMAC(sha,s,p,MCL_EFS,&F);

		s->len=len;
		MCL_OCT_copy(&U,&F);
		for (j=2;j<=rep;j++)
		{
			MCL_HMAC(sha,&U,p,MCL_EFS,&U);
			MCL_OCT_xor(&F,&U);
		}

		MCL_OCT_jmcl_octet(key,&F);
	}
	MCL_OCT_chop(key,NULL,olen);
}

/* AES encryption/decryption. Encrypt byte array M using key K and returns ciphertext */
void MCL_AES_CBC_IV0_ENCRYPT(mcl_octet *k,mcl_octet *m,mcl_octet *c)
{ /* AES CBC encryption, with Null IV and key k */
  /* Input is from an mcl_octet string m, output is to an mcl_octet string c */
  /* Input is padded as necessary to make up a full final block */
    mcl_aes a;
	int fin;
    int i,j,ipt,opt;
    char buff[16];
    int padlen;

	MCL_OCT_clear(c);
	if (m->len==0) return;
    MCL_AES_init(&a,CBC,k->len,k->val,NULL);

    ipt=opt=0;
    fin=0;
    for(;;)
    {
        for (i=0;i<16;i++)
        {
            if (ipt<m->len) buff[i]=m->val[ipt++];
            else {fin=1; break;}
        }
        if (fin) break;
        MCL_AES_encrypt(&a,buff);
        for (i=0;i<16;i++)
            if (opt<c->max) c->val[opt++]=buff[i];
    }    

/* last block, filled up to i-th index */

    padlen=16-i;
    for (j=i;j<16;j++) buff[j]=padlen;
    MCL_AES_encrypt(&a,buff);
    for (i=0;i<16;i++)
        if (opt<c->max) c->val[opt++]=buff[i];
    MCL_AES_end(&a);
    c->len=opt;    
}

/* decrypts and returns TRUE if all consistent, else returns FALSE */
int MCL_AES_CBC_IV0_DECRYPT(mcl_octet *k,mcl_octet *c,mcl_octet *m)
{ /* padding is removed */
    mcl_aes a;
    int i,ipt,opt,ch;
    char buff[16];
    int fin,bad;
    int padlen;
    ipt=opt=0;

    MCL_OCT_clear(m);
    if (c->len==0) return 1;
    ch=c->val[ipt++]; 
   
    MCL_AES_init(&a,CBC,k->len,k->val,NULL);
    fin=0;

    for(;;)
    {
        for (i=0;i<16;i++)
        {
            buff[i]=ch;      
            if (ipt>=c->len) {fin=1; break;}  
            else ch=c->val[ipt++];  
        }
        MCL_AES_decrypt(&a,buff);
        if (fin) break;
        for (i=0;i<16;i++)
            if (opt<m->max) m->val[opt++]=buff[i];
    }    
    MCL_AES_end(&a);
    bad=0;
    padlen=buff[15];
    if (i!=15 || padlen<1 || padlen>16) bad=1;
    if (padlen>=2 && padlen<=16)
        for (i=16-padlen;i<16;i++) if (buff[i]!=padlen) bad=1;
    
    if (!bad) for (i=0;i<16-padlen;i++)
        if (opt<m->max) m->val[opt++]=buff[i];
 
    m->len=opt;
    if (bad) return 0;
    return 1;
}

/* Calculate a public/private EC GF(p) key pair. W=S.G mod EC(p),
 * where S is the secret key and W is the public key
 * and G is fixed generator.
 * If RNG is NULL then the private key is provided externally in S
 * otherwise it is generated randomly internally */
int MCL_ECP_KEY_PAIR_GENERATE(csprng *RNG,mcl_octet* S,mcl_octet *W)
{
    mcl_chunk r[MCL_BS],gx[MCL_BS],gy[MCL_BS],s[MCL_BS];
    MCL_ECP G;
    int res=0;
	MCL_BIG_rcopy(gx,MCL_CURVE_Gx);
#if MCL_CURVETYPE!=MCL_MONTGOMERY
	MCL_BIG_rcopy(gy,MCL_CURVE_Gy);
    MCL_ECP_set(&G,gx,gy);
#else
    MCL_ECP_set(&G,gx);
#endif

	MCL_BIG_rcopy(r,MCL_CURVE_Order);
    if (RNG!=NULL)
		MCL_BIG_randomnum(s,r,RNG);
    else
	{
		MCL_BIG_fromBytes(s,S->val);
		MCL_BIG_mod(s,r);
	}

    MCL_ECP_mul(&G,s);
#if MCL_CURVETYPE!=MCL_MONTGOMERY
    MCL_ECP_get(gx,gy,&G);
#else
    MCL_ECP_get(gx,&G);
#endif
    if (RNG!=NULL) 
	{
		S->len=MCL_EGS;
		MCL_BIG_toBytes(S->val,s);
	}
#if MCL_CURVETYPE!=MCL_MONTGOMERY        
	W->len=2*MCL_EFS+1;	W->val[0]=4;
	MCL_BIG_toBytes(&(W->val[1]),gx);
	MCL_BIG_toBytes(&(W->val[MCL_EFS+1]),gy);
#else
	W->len=MCL_EFS+1;	W->val[0]=2;
	MCL_BIG_toBytes(&(W->val[1]),gx);
#endif

    return res;
}

/* validate public key. Set full=true for fuller check */
int MCL_ECP_PUBLIC_KEY_VALIDATE(int full,mcl_octet *W)
{
    mcl_chunk q[MCL_BS],r[MCL_BS],wx[MCL_BS],wy[MCL_BS];
    MCL_ECP WP;
    int valid;
    int res=0;

	MCL_BIG_rcopy(q,MCL_Modulus);
	MCL_BIG_rcopy(r,MCL_CURVE_Order);

	MCL_BIG_fromBytes(wx,&(W->val[1]));
    if (MCL_BIG_comp(wx,q)>=0) res=MCL_ECDH_INVALID_PUBLIC_KEY; 
#if MCL_CURVETYPE!=MCL_MONTGOMERY
	MCL_BIG_fromBytes(wy,&(W->val[MCL_EFS+1]));

	if (MCL_BIG_comp(wy,q)>=0) res=MCL_ECDH_INVALID_PUBLIC_KEY; 
#endif
    if (res==0)
    {
#if MCL_CURVETYPE!=MCL_MONTGOMERY
        valid=MCL_ECP_set(&WP,wx,wy);
#else
	    valid=MCL_ECP_set(&WP,wx);
#endif
        if (!valid || MCL_ECP_isinf(&WP)) res=MCL_ECDH_INVALID_PUBLIC_KEY;
        if (res==0 && full)
        {
            MCL_ECP_mul(&WP,r);
            if (!MCL_ECP_isinf(&WP)) res=MCL_ECDH_INVALID_PUBLIC_KEY; 
        }
    }

    return res;
}

/* IEEE-1363 Diffie-Hellman online calculation Z=S.WD */
int MCL_ECPSVDP_DH(mcl_octet *S,mcl_octet *WD,mcl_octet *Z)    
{
    mcl_chunk r[MCL_BS],s[MCL_BS],wx[MCL_BS],wy[MCL_BS];
    int valid;
    MCL_ECP W;
    int res=0;

	MCL_BIG_fromBytes(s,S->val);

	MCL_BIG_fromBytes(wx,&(WD->val[1]));
#if MCL_CURVETYPE!=MCL_MONTGOMERY
	MCL_BIG_fromBytes(wy,&(WD->val[MCL_EFS+1]));     
	valid=MCL_ECP_set(&W,wx,wy);
#else
	valid=MCL_ECP_set(&W,wx);
#endif
	if (!valid) res=MCL_ECDH_ERROR;
	if (res==0)
	{
		MCL_BIG_rcopy(r,MCL_CURVE_Order);
		MCL_BIG_mod(s,r);

	    MCL_ECP_mul(&W,s);
        if (MCL_ECP_isinf(&W)) res=MCL_ECDH_ERROR; 
        else
        {
#if MCL_CURVETYPE!=MCL_MONTGOMERY
            MCL_ECP_get(wx,wx,&W);
#else
	        MCL_ECP_get(wx,&W);
#endif
			Z->len=MCL_EFS;
			MCL_BIG_toBytes(Z->val,wx);
        }
    }
    return res;
}

#if MCL_CURVETYPE!=MCL_MONTGOMERY

/* IEEE ECDSA Signature, C and D are signature on F using private key S */
int MCL_ECPSP_DSA(int sha,csprng *RNG,mcl_octet *S,mcl_octet *F,mcl_octet *C,mcl_octet *D)
{
	char h[66];  // +2 is patch for MCL_NIST521
	mcl_octet H={0,sizeof(h),h};

    mcl_chunk gx[MCL_BS],gy[MCL_BS],r[MCL_BS],s[MCL_BS],f[MCL_BS],c[MCL_BS],d[MCL_BS],u[MCL_BS],vx[MCL_BS];
    MCL_ECP G,V;

	hashit(sha,F,-1,NULL,NULL,&H); 

	MCL_BIG_rcopy(gx,MCL_CURVE_Gx);
	MCL_BIG_rcopy(gy,MCL_CURVE_Gy);
	MCL_BIG_rcopy(r,MCL_CURVE_Order);
	
	MCL_BIG_fromBytes(s,S->val);
	if (MCL_MODBYTES>sha) MCL_OCT_shr(&H,MCL_MODBYTES-sha); // patch for MCL_NIST521

	MCL_BIG_fromBytesLen(f,H.val,H.len);
    MCL_ECP_set(&G,gx,gy);

    do {
		MCL_BIG_randomnum(u,r,RNG);
        MCL_ECP_copy(&V,&G);
        MCL_ECP_mul(&V,u);   
		
        MCL_ECP_get(vx,vx,&V);

		MCL_BIG_copy(c,vx);
		MCL_BIG_mod(c,r);
		if (MCL_BIG_iszilch(c)) continue;

		MCL_BIG_invmodp(u,u,r);

		MCL_BIG_modmul(d,s,c,r);

		MCL_BIG_add(d,f,d);

		MCL_BIG_modmul(d,u,d,r);

	} while (MCL_BIG_iszilch(d));
       
	C->len=D->len=MCL_EGS;

	MCL_BIG_toBytes(C->val,c);
	MCL_BIG_toBytes(D->val,d);

    return 0;
}

/* IEEE1363 ECDSA Signature Verification. Signature C and D on F is verified using public key W */
int MCL_ECPVP_DSA(int sha,mcl_octet *W,mcl_octet *F, mcl_octet *C,mcl_octet *D)
{
	char h[66];    // +2 is patch for MCL_NIST521
	mcl_octet H={0,sizeof(h),h};

    mcl_chunk r[MCL_BS],gx[MCL_BS],gy[MCL_BS],wx[MCL_BS],wy[MCL_BS],f[MCL_BS],c[MCL_BS],d[MCL_BS],h2[MCL_BS];
//	MCL_BIG inv,one;
    int res=0;
    MCL_ECP G,WP;
    int valid; 

 	hashit(sha,F,-1,NULL,NULL,&H); 

	MCL_BIG_rcopy(gx,MCL_CURVE_Gx);
	MCL_BIG_rcopy(gy,MCL_CURVE_Gy);
	MCL_BIG_rcopy(r,MCL_CURVE_Order);

	MCL_BIG_fromBytes(c,C->val);
	MCL_BIG_fromBytes(d,D->val);

	if (MCL_MODBYTES>sha) MCL_OCT_shr(&H,MCL_MODBYTES-sha); // patch for MCL_NIST521

	MCL_BIG_fromBytesLen(f,H.val,H.len);

    if (MCL_BIG_iszilch(c) || MCL_BIG_comp(c,r)>=0 || MCL_BIG_iszilch(d) || MCL_BIG_comp(d,r)>=0) 
		res=MCL_ECDH_INVALID;

    if (res==0)
    {
		MCL_BIG_invmodp(d,d,r);
		MCL_BIG_modmul(f,f,d,r);
		MCL_BIG_modmul(h2,c,d,r);

		MCL_ECP_set(&G,gx,gy);

		MCL_BIG_fromBytes(wx,&(W->val[1]));
		MCL_BIG_fromBytes(wy,&(W->val[MCL_EFS+1]));
 
		valid=MCL_ECP_set(&WP,wx,wy);

        if (!valid) res=MCL_ECDH_ERROR;
        else
        {
			MCL_ECP_mul2(&WP,&G,h2,f);

            if (MCL_ECP_isinf(&WP)) res=MCL_ECDH_INVALID;
            else
            {
                MCL_ECP_get(d,d,&WP);
				MCL_BIG_mod(d,r);
                if (MCL_BIG_comp(d,c)!=0) res=MCL_ECDH_INVALID;
            }
        }
    }

    return res;
}

/* IEEE1363 ECIES encryption. Encryption of plaintext M uses public key W and produces ciphertext V,C,T */
void MCL_ECP_ECIES_ENCRYPT(int sha,mcl_octet *P1,mcl_octet *P2,csprng *RNG,mcl_octet *W,mcl_octet *M,int tlen,mcl_octet *V,mcl_octet *C,mcl_octet *T)
{ 

	int i,len;
	char z[MCL_EFS],vz[3*MCL_EFS+2],k[2*MCL_EAS],k1[MCL_EAS],k2[MCL_EAS],l2[8],u[MCL_EFS];
	mcl_octet Z={0,sizeof(z),z};
	mcl_octet VZ={0,sizeof(vz),vz};
	mcl_octet K={0,sizeof(k),k};
	mcl_octet K1={0,sizeof(k1),k1};
	mcl_octet K2={0,sizeof(k2),k2};
	mcl_octet L2={0,sizeof(l2),l2};
	mcl_octet U={0,sizeof(u),u};

    if (MCL_ECP_KEY_PAIR_GENERATE(RNG,&U,V)!=0) return;  
    if (MCL_ECPSVDP_DH(&U,W,&Z)!=0) return;     

    MCL_OCT_copy(&VZ,V);
    MCL_OCT_jmcl_octet(&VZ,&Z);

	MCL_KDF2(sha,&VZ,P1,2*MCL_EAS,&K);

	K1.len=K2.len=MCL_EAS;
    for (i=0;i<MCL_EAS;i++) {K1.val[i]=K.val[i]; K2.val[i]=K.val[MCL_EAS+i];} 

	MCL_AES_CBC_IV0_ENCRYPT(&K1,M,C);
	
	MCL_OCT_jint(&L2,P2->len,8);

	len=C->len;
	MCL_OCT_jmcl_octet(C,P2);
    MCL_OCT_jmcl_octet(C,&L2);
	MCL_HMAC(sha,C,&K2,tlen,T);
	C->len=len;
}

/* IEEE1363 ECIES decryption. Decryption of ciphertext V,C,T using private key U outputs plaintext M */
int MCL_ECP_ECIES_DECRYPT(int sha,mcl_octet *P1,mcl_octet *P2,mcl_octet *V,mcl_octet *C,mcl_octet *T,mcl_octet *U,mcl_octet *M)
{ 

	int i,len;
	char z[MCL_EFS],vz[3*MCL_EFS+2],k[2*MCL_EAS],k1[MCL_EAS],k2[MCL_EAS],l2[8],tag[32];
	mcl_octet Z={0,sizeof(z),z};
	mcl_octet VZ={0,sizeof(vz),vz};
	mcl_octet K={0,sizeof(k),k};
	mcl_octet K1={0,sizeof(k1),k1};
	mcl_octet K2={0,sizeof(k2),k2};
	mcl_octet L2={0,sizeof(l2),l2};
	mcl_octet TAG={0,sizeof(tag),tag};

	if (MCL_ECPSVDP_DH(U,V,&Z)!=0) return 0;  

    MCL_OCT_copy(&VZ,V);
    MCL_OCT_jmcl_octet(&VZ,&Z);

	MCL_KDF2(sha,&VZ,P1,2*MCL_EAS,&K);

	K1.len=K2.len=MCL_EAS;
    for (i=0;i<MCL_EAS;i++) {K1.val[i]=K.val[i]; K2.val[i]=K.val[MCL_EAS+i];} 

	if (!MCL_AES_CBC_IV0_DECRYPT(&K1,C,M)) return 0;

	MCL_OCT_jint(&L2,P2->len,8);

	len=C->len;
	MCL_OCT_jmcl_octet(C,P2);
    MCL_OCT_jmcl_octet(C,&L2);	
	MCL_HMAC(sha,C,&K2,T->len,&TAG);
	C->len=len;

	if (!MCL_OCT_comp(T,&TAG)) return 0;
	
	return 1;

}

#endif
