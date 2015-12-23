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

/*
 * Implementation of the Secure Hashing Algorithm (SHA-1/256/384/512)
 *
 * Generates a 160/256/384/512 bit message digest. It should be impossible to come
 * come up with two messages that hash to the same value ("collision free").
 *
 * For use with byte-oriented messages only. Could/Should be speeded
 * up by unwinding loops in MCL_HASH_transform(), and assembly patches.
 */
/* SU=m, m is Stack Usage */

#include "mcl_arch.h"
#include "mcl_hash.h"

#define FIX

/* Include this #define in order to implement the
   rather mysterious 'fix' to SHS

   With this definition in, SHA-1 is implemented
   Without this definition, SHA-0 is implemented
*/


#define H0 0x67452301L
#define H1 0xefcdab89L
#define H2 0x98badcfeL
#define H3 0x10325476L
#define H4 0xc3d2e1f0L

#define K0 0x5a827999L
#define K1 0x6ed9eba1L
#define K2 0x8f1bbcdcL
#define K3 0xca62c1d6L

#define PAD  0x80
#define ZERO 0

/* functions */

#define SS(n,x) (((x)<<n) | ((x)>>(32-n)))

#define F0(x,y,z) (z^(x&(y^z)))
#define F1(x,y,z) (x^y^z)
#define F2(x,y,z) ((x&y) | (z&(x|y))) 
#define F3(x,y,z) (x^y^z)

static void MCL_HASH160_transform(mcl_hash160 *sh)
{ /* basic transformation step */
    unsign32 a,b,c,d,e,temp;
    int t;
#ifdef FIX
    for (t=16;t<80;t++) sh->w[t]=SS(1,sh->w[t-3]^sh->w[t-8]^sh->w[t-14]^sh->w[t-16]);
#else
    for (t=16;t<80;t++) sh->w[t]=sh->w[t-3]^sh->w[t-8]^sh->w[t-14]^sh->w[t-16];
#endif
    a=sh->h[0]; b=sh->h[1]; c=sh->h[2]; d=sh->h[3]; e=sh->h[4];
    for (t=0;t<20;t++)
    { /* 20 times - mush it up */
        temp=K0+F0(b,c,d)+SS(5,a)+e+sh->w[t];
        e=d; d=c;
        c=SS(30,b);
        b=a; a=temp;
    }
    for (t=20;t<40;t++)
    { /* 20 more times - mush it up */
        temp=K1+F1(b,c,d)+SS(5,a)+e+sh->w[t];
        e=d; d=c;
        c=SS(30,b);
        b=a; a=temp;
    }
    for (t=40;t<60;t++)
    { /* 20 more times - mush it up */
        temp=K2+F2(b,c,d)+SS(5,a)+e+sh->w[t];
        e=d; d=c;
        c=SS(30,b);
        b=a; a=temp;
    }
    for (t=60;t<80;t++)
    { /* 20 more times - mush it up */
        temp=K3+F3(b,c,d)+SS(5,a)+e+sh->w[t];
        e=d; d=c;
        c=SS(30,b);
        b=a; a=temp;
    }
    sh->h[0]+=a; sh->h[1]+=b; sh->h[2]+=c;
    sh->h[3]+=d; sh->h[4]+=e;
} 

void MCL_HASH160_init(mcl_hash160 *sh)
{ /* re-initialise */
    int i;
    for (i=0;i<80;i++) sh->w[i]=0L;
    sh->length[0]=sh->length[1]=0L;
    sh->h[0]=H0;
    sh->h[1]=H1;
    sh->h[2]=H2;
    sh->h[3]=H3;
    sh->h[4]=H4;

	sh->hlen=20;
}

void MCL_HASH160_process(mcl_hash160 *sh,int byte)
{ /* process the next message byte */
    int cnt;
    
    cnt=(int)((sh->length[0]/32)%16);
    
    sh->w[cnt]<<=8;
    sh->w[cnt]|=(unsign32)(byte&0xFF);

    sh->length[0]+=8;
    if (sh->length[0]==0L) { sh->length[1]++; sh->length[0]=0L; }
    if ((sh->length[0]%512)==0) MCL_HASH160_transform(sh);
}

void MCL_HASH160_hash(mcl_hash160 *sh,char *hash)
{ /* pad message and finish - supply digest */
    int i;
    unsign32 len0,len1;
    len0=sh->length[0];
    len1=sh->length[1];
    MCL_HASH160_process(sh,PAD);
    while ((sh->length[0]%512)!=448) MCL_HASH160_process(sh,ZERO);
    sh->w[14]=len1;
    sh->w[15]=len0;    
    MCL_HASH160_transform(sh);
    for (i=0;i<sh->hlen;i++)
    { /* convert to bytes */
        hash[i]=(char)((sh->h[i/4]>>(8*(3-i%4))) & 0xffL);
    }
    MCL_HASH160_init(sh);
}

#define H0_256 0x6A09E667L
#define H1_256 0xBB67AE85L
#define H2_256 0x3C6EF372L
#define H3_256 0xA54FF53AL
#define H4_256 0x510E527FL
#define H5_256 0x9B05688CL
#define H6_256 0x1F83D9ABL
#define H7_256 0x5BE0CD19L

static const unsign32 K_256[64]={
0x428a2f98L,0x71374491L,0xb5c0fbcfL,0xe9b5dba5L,0x3956c25bL,0x59f111f1L,0x923f82a4L,0xab1c5ed5L,
0xd807aa98L,0x12835b01L,0x243185beL,0x550c7dc3L,0x72be5d74L,0x80deb1feL,0x9bdc06a7L,0xc19bf174L,
0xe49b69c1L,0xefbe4786L,0x0fc19dc6L,0x240ca1ccL,0x2de92c6fL,0x4a7484aaL,0x5cb0a9dcL,0x76f988daL,
0x983e5152L,0xa831c66dL,0xb00327c8L,0xbf597fc7L,0xc6e00bf3L,0xd5a79147L,0x06ca6351L,0x14292967L,
0x27b70a85L,0x2e1b2138L,0x4d2c6dfcL,0x53380d13L,0x650a7354L,0x766a0abbL,0x81c2c92eL,0x92722c85L,
0xa2bfe8a1L,0xa81a664bL,0xc24b8b70L,0xc76c51a3L,0xd192e819L,0xd6990624L,0xf40e3585L,0x106aa070L,
0x19a4c116L,0x1e376c08L,0x2748774cL,0x34b0bcb5L,0x391c0cb3L,0x4ed8aa4aL,0x5b9cca4fL,0x682e6ff3L,
0x748f82eeL,0x78a5636fL,0x84c87814L,0x8cc70208L,0x90befffaL,0xa4506cebL,0xbef9a3f7L,0xc67178f2L};

#define PAD  0x80
#define ZERO 0

/* functions */

#define S(m,n,x) (((x)>>n) | ((x)<<(m-n)))
#define R(n,x) ((x)>>n)

#define Ch(x,y,z)  ((x&y)^(~(x)&z))
#define Maj(x,y,z) ((x&y)^(x&z)^(y&z))
#define Sig0_256(x)    (S(32,2,x)^S(32,13,x)^S(32,22,x))
#define Sig1_256(x)    (S(32,6,x)^S(32,11,x)^S(32,25,x))
#define theta0_256(x)  (S(32,7,x)^S(32,18,x)^R(3,x))
#define theta1_256(x)  (S(32,17,x)^S(32,19,x)^R(10,x))

#define Sig0_512(x)    (S(64,28,x)^S(64,34,x)^S(64,39,x))
#define Sig1_512(x)    (S(64,14,x)^S(64,18,x)^S(64,41,x))
#define theta0_512(x)  (S(64,1,x)^S(64,8,x)^R(7,x))
#define theta1_512(x)  (S(64,19,x)^S(64,61,x)^R(6,x))



/* SU= 72 */
static void MCL_HASH256_transform(mcl_hash256 *sh)
{ /* basic transformation step */
    unsign32 a,b,c,d,e,f,g,h,t1,t2;
    int j;
    for (j=16;j<64;j++) 
        sh->w[j]=theta1_256(sh->w[j-2])+sh->w[j-7]+theta0_256(sh->w[j-15])+sh->w[j-16];

    a=sh->h[0]; b=sh->h[1]; c=sh->h[2]; d=sh->h[3]; 
    e=sh->h[4]; f=sh->h[5]; g=sh->h[6]; h=sh->h[7];

    for (j=0;j<64;j++)
    { /* 64 times - mush it up */
        t1=h+Sig1_256(e)+Ch(e,f,g)+K_256[j]+sh->w[j];
        t2=Sig0_256(a)+Maj(a,b,c);
        h=g; g=f; f=e;
        e=d+t1;
        d=c;
        c=b;
        b=a;
        a=t1+t2;        
    }

    sh->h[0]+=a; sh->h[1]+=b; sh->h[2]+=c; sh->h[3]+=d; 
    sh->h[4]+=e; sh->h[5]+=f; sh->h[6]+=g; sh->h[7]+=h; 
} 

/* Initialise Hash function */
void MCL_HASH256_init(mcl_hash256 *sh)
{ /* re-initialise */
    int i;
    for (i=0;i<64;i++) sh->w[i]=0L;
    sh->length[0]=sh->length[1]=0L;
    sh->h[0]=H0_256;
    sh->h[1]=H1_256;
    sh->h[2]=H2_256;
    sh->h[3]=H3_256;
    sh->h[4]=H4_256;
    sh->h[5]=H5_256;
    sh->h[6]=H6_256;
    sh->h[7]=H7_256;

	sh->hlen=32;
}

/* process a single byte */
void MCL_HASH256_process(mcl_hash256 *sh,int byte)
{ /* process the next message byte */
    int cnt;
//printf("byt= %x\n",byte);
    cnt=(int)((sh->length[0]/32)%16);
    
    sh->w[cnt]<<=8;
    sh->w[cnt]|=(unsign32)(byte&0xFF);

    sh->length[0]+=8;
    if (sh->length[0]==0L) { sh->length[1]++; sh->length[0]=0L; }
    if ((sh->length[0]%512)==0) MCL_HASH256_transform(sh);
}

/* SU= 24 */
/* Generate 32-byte Hash */
void MCL_HASH256_hash(mcl_hash256 *sh,char *digest)
{ /* pad message and finish - supply digest */
    int i;
    unsign32 len0,len1;
    len0=sh->length[0];
    len1=sh->length[1];
    MCL_HASH256_process(sh,PAD);
    while ((sh->length[0]%512)!=448) MCL_HASH256_process(sh,ZERO);
    sh->w[14]=len1;
    sh->w[15]=len0;    
    MCL_HASH256_transform(sh);
    for (i=0;i<sh->hlen;i++)
    { /* convert to bytes */
        digest[i]=(char)((sh->h[i/4]>>(8*(3-i%4))) & 0xffL);
    }
    MCL_HASH256_init(sh);
}


#define H0_512 0x6a09e667f3bcc908 
#define H1_512 0xbb67ae8584caa73b 
#define H2_512 0x3c6ef372fe94f82b 
#define H3_512 0xa54ff53a5f1d36f1 
#define H4_512 0x510e527fade682d1 
#define H5_512 0x9b05688c2b3e6c1f 
#define H6_512 0x1f83d9abfb41bd6b 
#define H7_512 0x5be0cd19137e2179 

#define H8_512 0xcbbb9d5dc1059ed8 
#define H9_512 0x629a292a367cd507 
#define HA_512 0x9159015a3070dd17 
#define HB_512 0x152fecd8f70e5939 
#define HC_512 0x67332667ffc00b31 
#define HD_512 0x8eb44a8768581511 
#define HE_512 0xdb0c2e0d64f98fa7 
#define HF_512 0x47b5481dbefa4fa4 

/* */

static const unsign64 K_512[80]=
{0x428a2f98d728ae22 ,0x7137449123ef65cd ,0xb5c0fbcfec4d3b2f ,0xe9b5dba58189dbbc ,
0x3956c25bf348b538 ,0x59f111f1b605d019 ,0x923f82a4af194f9b ,0xab1c5ed5da6d8118 ,
0xd807aa98a3030242 ,0x12835b0145706fbe ,0x243185be4ee4b28c ,0x550c7dc3d5ffb4e2 ,
0x72be5d74f27b896f ,0x80deb1fe3b1696b1 ,0x9bdc06a725c71235 ,0xc19bf174cf692694 ,
0xe49b69c19ef14ad2 ,0xefbe4786384f25e3 ,0x0fc19dc68b8cd5b5 ,0x240ca1cc77ac9c65 ,
0x2de92c6f592b0275 ,0x4a7484aa6ea6e483 ,0x5cb0a9dcbd41fbd4 ,0x76f988da831153b5 ,
0x983e5152ee66dfab ,0xa831c66d2db43210 ,0xb00327c898fb213f ,0xbf597fc7beef0ee4 ,
0xc6e00bf33da88fc2 ,0xd5a79147930aa725 ,0x06ca6351e003826f ,0x142929670a0e6e70 ,
0x27b70a8546d22ffc ,0x2e1b21385c26c926 ,0x4d2c6dfc5ac42aed ,0x53380d139d95b3df ,
0x650a73548baf63de ,0x766a0abb3c77b2a8 ,0x81c2c92e47edaee6 ,0x92722c851482353b ,
0xa2bfe8a14cf10364 ,0xa81a664bbc423001 ,0xc24b8b70d0f89791 ,0xc76c51a30654be30 ,
0xd192e819d6ef5218 ,0xd69906245565a910 ,0xf40e35855771202a ,0x106aa07032bbd1b8 ,
0x19a4c116b8d2d0c8 ,0x1e376c085141ab53 ,0x2748774cdf8eeb99 ,0x34b0bcb5e19b48a8 ,
0x391c0cb3c5c95a63 ,0x4ed8aa4ae3418acb ,0x5b9cca4f7763e373 ,0x682e6ff3d6b2b8a3 ,
0x748f82ee5defb2fc ,0x78a5636f43172f60 ,0x84c87814a1f0ab72 ,0x8cc702081a6439ec ,
0x90befffa23631e28 ,0xa4506cebde82bde9 ,0xbef9a3f7b2c67915 ,0xc67178f2e372532b ,
0xca273eceea26619c ,0xd186b8c721c0c207 ,0xeada7dd6cde0eb1e ,0xf57d4f7fee6ed178 ,
0x06f067aa72176fba ,0x0a637dc5a2c898a6 ,0x113f9804bef90dae ,0x1b710b35131c471b ,
0x28db77f523047d84 ,0x32caab7b40c72493 ,0x3c9ebe0a15c9bebc ,0x431d67c49c100d4c ,
0x4cc5d4becb3e42b6 ,0x597f299cfc657e2a ,0x5fcb6fab3ad6faec ,0x6c44198c4a475817 };


static void MCL_HASH512_transform(mcl_hash512 *sh)
{ /* basic transformation step */
    unsign64 a,b,c,d,e,f,g,h,t1,t2;
    int j;
    for (j=16;j<80;j++) 
        sh->w[j]=theta1_512(sh->w[j-2])+sh->w[j-7]+theta0_512(sh->w[j-15])+sh->w[j-16];

    a=sh->h[0]; b=sh->h[1]; c=sh->h[2]; d=sh->h[3]; 
    e=sh->h[4]; f=sh->h[5]; g=sh->h[6]; h=sh->h[7];

    for (j=0;j<80;j++)
    { /* 80 times - mush it up */
        t1=h+Sig1_512(e)+Ch(e,f,g)+K_512[j]+sh->w[j];
        t2=Sig0_512(a)+Maj(a,b,c);
        h=g; g=f; f=e;
        e=d+t1;
        d=c;
        c=b;
        b=a;
        a=t1+t2;        
    }
    sh->h[0]+=a; sh->h[1]+=b; sh->h[2]+=c; sh->h[3]+=d; 
    sh->h[4]+=e; sh->h[5]+=f; sh->h[6]+=g; sh->h[7]+=h; 
} 

void MCL_HASH384_init(mcl_hash384 *sh)
{ /* re-initialise */
    int i;
    for (i=0;i<80;i++) sh->w[i]=0;
    sh->length[0]=sh->length[1]=0;
    sh->h[0]=H8_512;
    sh->h[1]=H9_512;
    sh->h[2]=HA_512;
    sh->h[3]=HB_512;
    sh->h[4]=HC_512;
    sh->h[5]=HD_512;
    sh->h[6]=HE_512;
    sh->h[7]=HF_512;

	sh->hlen=48;

}

void MCL_HASH384_process(mcl_hash384 *sh,int byte)
{ /* process the next message byte */
	MCL_HASH512_process(sh,byte);
}

void MCL_HASH384_hash(mcl_hash384 *sh,char *hash)
{ /* pad message and finish - supply digest */
	MCL_HASH512_hash(sh,hash);
}

void MCL_HASH512_init(mcl_hash512 *sh)
{ /* re-initialise */
    int i;

    for (i=0;i<80;i++) sh->w[i]=0;
    sh->length[0]=sh->length[1]=0;
    sh->h[0]=H0_512;
    sh->h[1]=H1_512;
    sh->h[2]=H2_512;
    sh->h[3]=H3_512;
    sh->h[4]=H4_512;
    sh->h[5]=H5_512;
    sh->h[6]=H6_512;
    sh->h[7]=H7_512;

	sh->hlen=64;
}

void MCL_HASH512_process(mcl_hash512 *sh,int byte)
{ /* process the next message byte */
    int cnt;
    
    cnt=(int)((sh->length[0]/64)%16);
    
    sh->w[cnt]<<=8;
    sh->w[cnt]|=(unsign64)(byte&0xFF);

    sh->length[0]+=8;
    if (sh->length[0]==0L) { sh->length[1]++; sh->length[0]=0L; }
    if ((sh->length[0]%1024)==0) MCL_HASH512_transform(sh);
}

void MCL_HASH512_hash(mcl_hash512 *sh,char *hash)
{ /* pad message and finish - supply digest */
    int i;
    unsign64 len0,len1;
    len0=sh->length[0];
    len1=sh->length[1];
    MCL_HASH512_process(sh,PAD);
    while ((sh->length[0]%1024)!=896) MCL_HASH512_process(sh,ZERO);
    sh->w[14]=len1;
    sh->w[15]=len0;    
    MCL_HASH512_transform(sh);
    for (i=0;i<sh->hlen;i++)
    { /* convert to bytes */
        hash[i]=(char)((sh->h[i/8]>>(8*(7-i%8))) & 0xffL);
    }
    MCL_HASH512_init(sh);
}
