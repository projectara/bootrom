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

/* ARAcrypt basic functions for Large Finite Field support */

#include "mcl_arch.h"
#include "mcl_config.h"
#include "mcl_big.h"
#include "mcl_ff.h"

#define MCL_MODBYTES (1+(MCL_MBITS-1)/8) /**< Number of bytes in MCL_Modulus */
#define MCL_NLEN (1+((MCL_MBITS-1)/MCL_BASEBITS))	/**< Number of words in MCL_BIG. */
#define MCL_BIGBITS (MCL_MODBYTES*8) /**< Number of bits representable in a MCL_BIG */
#define MCL_FF_BITS (MCL_BIGBITS*MCL_FFLEN) /**< Finite Field Size in bits - must be MCL_BIGBITS.2^n */

#define BMASK (((mcl_chunk)1<<MCL_BASEBITS)-1) /**< Mask = 2^MCL_BASEBITS-1 */

#define P_MCL_MBITS (MCL_MODBYTES*8)
#define P_MB (P_MCL_MBITS%MCL_BASEBITS)
#define P_OMASK ((mcl_chunk)(-1)<<(P_MCL_MBITS%MCL_BASEBITS))
#define P_EXCESS(a) ((a[MCL_NLEN-1]&P_OMASK)>>(P_MB))
#define P_FEXCESS ((mcl_chunk)1<<(MCL_BASEBITS*MCL_NLEN-P_MCL_MBITS))
#define P_TBITS (P_MCL_MBITS%MCL_BASEBITS)

/* set x = x mod 2^m */
static void MCL_BIG_mod2m(MCL_BIG x,int m)
{
	int i,wd,bt;
	mcl_chunk msk;
//	if (m>=MODBITS) return;
	wd=m/MCL_BASEBITS;
	bt=m%MCL_BASEBITS;
	msk=((mcl_chunk)1<<bt)-1;
	x[wd]&=msk;
	for (i=wd+1;i<MCL_NLEN;i++) x[i]=0;
}

/* Arazi and Qi inversion mod 256 */
static int invmod256(int a)
{
	int U,t1,t2,b,c;
	t1=0;
	c=(a>>1)&1;  
	t1+=c;
	t1&=1;
	t1=2-t1;
	t1<<=1;
	U=t1+1;

// i=2
	b=a&3;
	t1=U*b; t1>>=2;
	c=(a>>2)&3;
	t2=(U*c)&3;
	t1+=t2;
	t1*=U; t1&=3;
	t1=4-t1;
	t1<<=2;
	U+=t1;

// i=4
	b=a&15;
	t1=U*b; t1>>=4;
	c=(a>>4)&15;
	t2=(U*c)&15;
	t1+=t2;
	t1*=U; t1&=15;
	t1=16-t1;
	t1<<=4;
	U+=t1;

	return U;
}

/* a=1/a mod 2^MCL_BIGBITS. This is very fast! */
void MCL_BIG_invmod2m(MCL_BIG a)
{
	int i;
	mcl_chunk U[MCL_BS],t1[MCL_BS],b[MCL_BS],c[MCL_BS];
	MCL_BIG_zero(U);
	MCL_BIG_inc(U,invmod256(MCL_BIG_lastbits(a,8)));

	for (i=8;i<MCL_BIGBITS;i<<=1)
	{
		MCL_BIG_copy(b,a); MCL_BIG_mod2m(b,i);   // bottom i bits of a
		MCL_BIG_smul(t1,U,b); MCL_BIG_shr(t1,i); // top i bits of U*b
		MCL_BIG_copy(c,a); MCL_BIG_shr(c,i); MCL_BIG_mod2m(c,i); // top i bits of a
		MCL_BIG_smul(b,U,c); MCL_BIG_mod2m(b,i);  // bottom i bits of U*c
		MCL_BIG_add(t1,t1,b);
		MCL_BIG_smul(b,t1,U); MCL_BIG_copy(t1,b);  // (t1+b)*U

		MCL_BIG_mod2m(t1,i);				// bottom i bits of (t1+b)*U
		MCL_BIG_one(b); MCL_BIG_shl(b,i); MCL_BIG_sub(t1,b,t1); MCL_BIG_norm(t1);
		MCL_BIG_shl(t1,i);
		MCL_BIG_add(U,U,t1);
	}

	MCL_BIG_copy(a,U);

}

/* 
void FF_rcopy(MCL_BIG x[],const MCL_BIG y[],int n)
{
	int i;
	for (i=0;i<n;i++)
		MCL_BIG_rcopy(x[i],y[i]);
}
*/

/* x=y */
void MCL_FF_copy(mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],int n)
{
	int i;
	for (i=0;i<n;i++)
		MCL_BIG_copy(x[i],y[i]);
}

/* x=y<<n */
static void FF_dsucopy(mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],int n)
{
	int i;
	for (i=0;i<n;i++)
	{
		MCL_BIG_copy(x[n+i],y[i]);
		MCL_BIG_zero(x[i]);
	}
}

/* x=y */
static void FF_dscopy(mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],int n)
{
	int i;
	for (i=0;i<n;i++)
	{
		MCL_BIG_copy(x[i],y[i]);
		MCL_BIG_zero(x[n+i]);
	}
}

/* x=y>>n */
static void FF_sducopy(mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],int n)
{
	int i;
	for (i=0;i<n;i++)
		MCL_BIG_copy(x[i],y[n+i]);
}

/* set to zero */
void MCL_FF_zero(mcl_chunk x[][MCL_BS],int n)
{
	int i;
	for (i=0;i<n;i++)
		MCL_BIG_zero(x[i]);
}

/* test equals 0 */
int MCL_FF_iszilch(mcl_chunk x[][MCL_BS],int n)
{
	int i;
	for (i=0;i<n;i++)
		if (!MCL_BIG_iszilch(x[i])) return 0;
	return 1;
}

/* shift right by MCL_BIGBITS-bit words */
static void MCL_FF_shrw(mcl_chunk a[][MCL_BS],int n)
{
	int i;
	for (i=0;i<n;i++) {MCL_BIG_copy(a[i],a[i+n]);MCL_BIG_zero(a[i+n]);}
}

/* shift left by MCL_BIGBITS-bit words */
static void MCL_FF_shlw(mcl_chunk a[][MCL_BS],int n)
{
	int i;
	for (i=0;i<n;i++) {MCL_BIG_copy(a[i+n],a[i]); MCL_BIG_zero(a[i]);}
}

/* extract last bit */
int MCL_FF_parity(mcl_chunk x[][MCL_BS])
{
	return MCL_BIG_parity(x[0]);
}

/* extract last m bits */
int MCL_FF_lastbits(mcl_chunk x[][MCL_BS],int m)
{
	return MCL_BIG_lastbits(x[0],m);
}

/* x=1 */
void MCL_FF_one(mcl_chunk x[][MCL_BS],int n)
{
	int i;
	MCL_BIG_one(x[0]);
	for (i=1;i<n;i++)
		MCL_BIG_zero(x[i]);
}

/* x=m, where m is 32-bit int */
void MCL_FF_init(mcl_chunk x[][MCL_BS],sign32 m,int n)
{
	int i;
	MCL_BIG_zero(x[0]); 
#if MCL_CHUNK<64
	x[0][0]=(mcl_chunk)(m&BMASK);
	x[0][1]=(mcl_chunk)(m>>MCL_BASEBITS);
#else
	x[0][0]=(mcl_chunk)m;
#endif
	for (i=1;i<n;i++)
		MCL_BIG_zero(x[i]);
}

/* compare x and y - must be normalised */
int MCL_FF_comp(mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],int n)
{
	int i,j;
	for (i=n-1;i>=0;i--)
	{
		j=MCL_BIG_comp(x[i],y[i]);
		if (j!=0) return j;
	}
	return 0;
}

/* recursive add */
static void FF_radd(mcl_chunk z[][MCL_BS],int zp,mcl_chunk x[][MCL_BS],int xp,mcl_chunk y[][MCL_BS],int yp,int n)
{
	int i;
	for (i=0;i<n;i++)
		MCL_BIG_add(z[zp+i],x[xp+i],y[yp+i]);
}

/* recursive inc */
static void FF_rinc(mcl_chunk z[][MCL_BS],int zp,mcl_chunk y[][MCL_BS],int yp,int n)
{
	int i;
	for (i=0;i<n;i++)
		MCL_BIG_add(z[zp+i],z[zp+i],y[yp+i]);
}

/* recursive sub */
/* static void FF_rsub(MCL_BIG z[],int zp,MCL_BIG x[],int xp,MCL_BIG y[],int yp,int n) */
/* { */
/* 	int i; */
/* 	for (i=0;i<n;i++) */
/* 		MCL_BIG_sub(z[zp+i],x[xp+i],y[yp+i]); */
/* } */

/* recursive dec */
static void FF_rdec(mcl_chunk z[][MCL_BS],int zp,mcl_chunk y[][MCL_BS],int yp,int n)
{
	int i;
	for (i=0;i<n;i++)
		MCL_BIG_sub(z[zp+i],z[zp+i],y[yp+i]);
}

/* simple add */
void MCL_FF_add(mcl_chunk z[][MCL_BS],mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],int n)
{
	int i;
	for (i=0;i<n;i++)
		MCL_BIG_add(z[i],x[i],y[i]);
}

/* simple sub */
void MCL_FF_sub(mcl_chunk z[][MCL_BS],mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],int n)
{
	int i;
	for (i=0;i<n;i++)
		MCL_BIG_sub(z[i],x[i],y[i]);
}

/* increment/decrement by a small integer */
void MCL_FF_inc(mcl_chunk x[][MCL_BS],int m,int n)
{
	MCL_BIG_inc(x[0],m);
	MCL_FF_norm(x,n);
}

void MCL_FF_dec(mcl_chunk x[][MCL_BS],int m,int n)
{
	MCL_BIG_dec(x[0],m);
	MCL_FF_norm(x,n);
}

/* normalise - but hold any overflow in top part unless n<0 */
static void FF_rnorm(mcl_chunk z[][MCL_BS],int zp,int n)
{
	int i,trunc=0;	
	mcl_chunk carry;
	if (n<0)
	{ /* -v n signals to do truncation */
		n=-n;
		trunc=1;
	}
	for (i=0;i<n-1;i++)
	{
		carry=MCL_BIG_norm(z[zp+i]);
		z[zp+i][MCL_NLEN-1]^=carry<<P_TBITS; /* remove it */
		z[zp+i+1][0]+=carry;
	}
	carry=MCL_BIG_norm(z[zp+n-1]);
	if (trunc) z[zp+n-1][MCL_NLEN-1]^=carry<<P_TBITS;
}

void MCL_FF_norm(mcl_chunk z[][MCL_BS],int n)
{
	FF_rnorm(z,0,n);
}

/* shift left by one bit */
void MCL_FF_shl(mcl_chunk x[][MCL_BS],int n)
{
	int i;
	mcl_chunk carry,delay_carry=0;
	for (i=0;i<n-1;i++)
	{
		carry=MCL_BIG_fshl(x[i],1);
		x[i][0]|=delay_carry;
		x[i][MCL_NLEN-1]^=carry<<P_TBITS;
		delay_carry=carry;
	}
	MCL_BIG_fshl(x[n-1],1);
	x[n-1][0]|=delay_carry;
}

/* shift right by one bit */
void MCL_FF_shr(mcl_chunk x[][MCL_BS],int n)
{
	int i;
	mcl_chunk carry;
	for (i=n-1;i>0;i--)
	{
		carry=MCL_BIG_fshr(x[i],1);
		x[i-1][MCL_NLEN-1]|=carry<<P_TBITS;
	}
	MCL_BIG_fshr(x[0],1);
}

void MCL_FF_output(mcl_chunk x[][MCL_BS],int n)
{
	int i;
	MCL_FF_norm(x,n);
	for (i=n-1;i>=0;i--)
	{
		MCL_BIG_output(x[i]);// printf(" ");
	}
}

/* Convert FFs to/from mcl_octet strings */
void MCL_FF_toOctet(mcl_octet *w,mcl_chunk x[][MCL_BS],int n)
{
	int i;
	w->len=n*MCL_MODBYTES;
	for (i=0;i<n;i++)
	{
		MCL_BIG_toBytes(&(w->val[(n-i-1)*MCL_MODBYTES]),x[i]);
	}
}

void MCL_FF_fromOctet(mcl_chunk x[][MCL_BS],mcl_octet *w,int n)
{
	int i;
	for (i=0;i<n;i++)
	{
		MCL_BIG_fromBytes(x[i],&(w->val[(n-i-1)*MCL_MODBYTES]));
	}
}

/* in-place swapping using xor - side channel resistant */
static void FF_cswap(mcl_chunk a[][MCL_BS],mcl_chunk b[][MCL_BS],int d,int n)
{
	int i;
	for (i=0;i<n;i++)
		MCL_BIG_cswap(a[i],b[i],d);
	return;
}

/* z=x*y, t is workspace */
static void FF_karmul(mcl_chunk z[][MCL_BS],int zp,mcl_chunk x[][MCL_BS],int xp,mcl_chunk y[][MCL_BS],int yp,mcl_chunk t[][MCL_BS],int tp,int n)
{
    int nd2;
	if (n==1)
	{
		MCL_BIG_mul(t[tp],x[xp],y[yp]);
		MCL_BIG_split(z[zp+1],z[zp],t[tp],MCL_BIGBITS);
		return;
	}

	nd2=n/2;
	FF_radd(z,zp,x,xp,x,xp+nd2,nd2); 
//#if MCL_CHUNK<64
	FF_rnorm(z,zp,nd2);  /* needs this if recursion level too deep */
//#endif
	FF_radd(z,zp+nd2,y,yp,y,yp+nd2,nd2);  
//#if MCL_CHUNK<64
	FF_rnorm(z,zp+nd2,nd2);
//#endif
	FF_karmul(t,tp,z,zp,z,zp+nd2,t,tp+n,nd2);   
	FF_karmul(z,zp,x,xp,y,yp,t,tp+n,nd2);   
	FF_karmul(z,zp+n,x,xp+nd2,y,yp+nd2,t,tp+n,nd2);  
	FF_rdec(t,tp,z,zp,n);  
	FF_rdec(t,tp,z,zp+n,n);  
	FF_rinc(z,zp+nd2,t,tp,n);	
	FF_rnorm(z,zp,2*n);
}

static void FF_karsqr(mcl_chunk z[][MCL_BS],int zp,mcl_chunk x[][MCL_BS],int xp,mcl_chunk t[][MCL_BS],int tp,int n)
{
	int nd2;
	if (n==1)
	{
		MCL_BIG_sqr(t[tp],x[xp]);
		MCL_BIG_split(z[zp+1],z[zp],t[tp],MCL_BIGBITS);
		return;
	}
	nd2=n/2;
	FF_karsqr(z,zp,x,xp,t,tp+n,nd2);
	FF_karsqr(z,zp+n,x,xp+nd2,t,tp+n,nd2);
	FF_karmul(t,tp,x,xp,x,xp+nd2,t,tp+n,nd2);
	FF_rinc(z,zp+nd2,t,tp,n);
	FF_rinc(z,zp+nd2,t,tp,n);

	FF_rnorm(z,zp+nd2,n);  /* was FF_rnorm(z,zp,2*n)  */
}

static void FF_karmul_lower(mcl_chunk z[][MCL_BS],int zp,mcl_chunk x[][MCL_BS],int xp,mcl_chunk y[][MCL_BS],int yp,mcl_chunk t[][MCL_BS],int tp,int n)
{ /* Calculates Least Significant bottom half of x*y */
    int nd2;
    if (n==1)
    { /* only calculate bottom half of product */
	//	MCL_BIG_mul(d,x[xp],y[yp]);
	//	MCL_BIG_split(z[zp],z[zp],d,MCL_BIGBITS);
		MCL_BIG_smul(z[zp],x[xp],y[yp]);
        return;
    }
    nd2=n/2;

	FF_karmul(z,zp,x,xp,y,yp,t,tp+n,nd2);
	FF_karmul_lower(t,tp,x,xp+nd2,y,yp,t,tp+n,nd2);
	FF_rinc(z,zp+nd2,t,tp,nd2);
	FF_karmul_lower(t,tp,x,xp,y,yp+nd2,t,tp+n,nd2);
	FF_rinc(z,zp+nd2,t,tp,nd2);
	FF_rnorm(z,zp+nd2,-nd2);  /* truncate it */
}

static void FF_karmul_upper(mcl_chunk z[][MCL_BS],mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],mcl_chunk t[][MCL_BS],int n)
{ /* Calculates Most Significant upper half of x*y, given lower part */
    int nd2;
 
    nd2=n/2;
    FF_radd(z,n,x,0,x,nd2,nd2);  
    FF_radd(z,n+nd2,y,0,y,nd2,nd2);

    FF_karmul(t,0,z,n+nd2,z,n,t,n,nd2);  /* t = (a0+a1)(b0+b1) */
    FF_karmul(z,n,x,nd2,y,nd2,t,n,nd2);  /* z[n]= a1*b1 */
    /* z[0-nd2]=l(a0b0) z[nd2-n]= h(a0b0)+l(t)-l(a0b0)-l(a1b1) */
    FF_rdec(t,0,z,n,n);       /* t=t-a1b1  */							
    FF_rinc(z,nd2,z,0,nd2);   /* z[nd2-n]+=l(a0b0) = h(a0b0)+l(t)-l(a1b1)  */
    FF_rdec(z,nd2,t,0,nd2);   /* z[nd2-n]=h(a0b0)+l(t)-l(a1b1)-l(t-a1b1)=h(a0b0) */
    FF_rnorm(z,0,-n);	      /* a0b0 now in z - truncate it */
    FF_rdec(t,0,z,0,n);       /* (a0+a1)(b0+b1) - a0b0 */
    FF_rinc(z,nd2,t,0,n);

    FF_rnorm(z,nd2,n);
}

/* z=x*y */
void MCL_FF_mul(mcl_chunk z[][MCL_BS],mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],int n)
{
#ifndef C99
	mcl_chunk t[2*MCL_FFLEN][MCL_BS];
#else
	mcl_chunk t[2*n][MCL_BS];
#endif
	FF_karmul(z,0,x,0,y,0,t,0,n);
}

/* return low part of product */
static void FF_lmul(mcl_chunk z[][MCL_BS],mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],int n)
{
#ifndef C99
	mcl_chunk t[2*MCL_FFLEN][MCL_BS];
#else
	mcl_chunk t[2*n][MCL_BS];
#endif
	FF_karmul_lower(z,0,x,0,y,0,t,0,n);
}

/* Set b=b mod c */
void MCL_FF_mod(mcl_chunk b[][MCL_BS],mcl_chunk c[][MCL_BS],int n)
{
	int k=0;  

	MCL_FF_norm(b,n);
	if (MCL_FF_comp(b,c,n)<0) 
		return;
	do
	{
		MCL_FF_shl(c,n);
		k++;
	} while (MCL_FF_comp(b,c,n)>=0);

	while (k>0)
	{
		MCL_FF_shr(c,n);
		if (MCL_FF_comp(b,c,n)>=0)
		{
			MCL_FF_sub(b,b,c,n);
			MCL_FF_norm(b,n);
		}
		k--;
	}
}

/* z=x^2 */
void MCL_FF_sqr(mcl_chunk z[][MCL_BS],mcl_chunk x[][MCL_BS],int n)
{
#ifndef C99
	mcl_chunk t[2*MCL_FFLEN][MCL_BS];
#else
	mcl_chunk t[2*n][MCL_BS];
#endif
	FF_karsqr(z,0,x,0,t,0,n);
}

/* r=t mod modulus, N is modulus, ND is Montgomery Constant */
static void FF_reduce(mcl_chunk r[][MCL_BS],mcl_chunk T[][MCL_BS],mcl_chunk N[][MCL_BS],mcl_chunk ND[][MCL_BS],int n)
{ /* fast karatsuba Montgomery reduction */
#ifndef C99
	mcl_chunk t[2*MCL_FFLEN][MCL_BS];
	mcl_chunk m[MCL_FFLEN][MCL_BS];
#else
	mcl_chunk t[2*n][MCL_BS];
	mcl_chunk m[n][MCL_BS];
#endif
	FF_sducopy(r,T,n);  /* keep top half of T */
	FF_karmul_lower(m,0,T,0,ND,0,t,0,n);  /* m=T.(1/N) mod R */

	FF_karmul_upper(T,N,m,t,n);  /* T=mN */
	FF_sducopy(m,T,n);

	MCL_FF_add(r,r,N,n);
	MCL_FF_sub(r,r,m,n);
	MCL_FF_norm(r,n);
}


/* Set r=a mod b */
/* a is of length - 2*n */
/* r,b is of length - n */
void MCL_FF_dmod(mcl_chunk r[][MCL_BS],mcl_chunk a[][MCL_BS],mcl_chunk b[][MCL_BS],int n)
{
	int k; 
#ifndef C99	
	mcl_chunk m[2*MCL_FFLEN][MCL_BS];
	mcl_chunk x[2*MCL_FFLEN][MCL_BS];
#else
	mcl_chunk m[2*n][MCL_BS];
	mcl_chunk x[2*n][MCL_BS];
#endif
	MCL_FF_copy(x,a,2*n);
	MCL_FF_norm(x,2*n);
	FF_dsucopy(m,b,n); k=MCL_BIGBITS*n;

	while (k>0)
	{
	//	len=2*n-((256*n-k)/256);  // reduce length as numbers get smaller?
		MCL_FF_shr(m,2*n);

		if (MCL_FF_comp(x,m,2*n)>=0)
		{
			MCL_FF_sub(x,x,m,2*n);
			MCL_FF_norm(x,2*n);
		}

		k--;
	}
	MCL_FF_copy(r,x,n);
	MCL_FF_mod(r,b,n);
}

/* Set r=1/a mod p. Binary method - a<p on entry */

void MCL_FF_invmodp(mcl_chunk r[][MCL_BS],mcl_chunk a[][MCL_BS],mcl_chunk p[][MCL_BS],int n)
{
#ifndef C99
	mcl_chunk u[MCL_FFLEN][MCL_BS],v[MCL_FFLEN][MCL_BS],x1[MCL_FFLEN][MCL_BS],x2[MCL_FFLEN][MCL_BS],t[MCL_FFLEN][MCL_BS],one[MCL_FFLEN][MCL_BS];
#else
	mcl_chunk u[n][MCL_BS],v[n][MCL_BS],x1[n][MCL_BS],x2[n][MCL_BS],t[n][MCL_BS],one[n][MCL_BS];
#endif
	MCL_FF_copy(u,a,n);
	MCL_FF_copy(v,p,n);
	MCL_FF_one(one,n);
	MCL_FF_copy(x1,one,n);
	MCL_FF_zero(x2,n);

// reduce n in here as well! 
	while (MCL_FF_comp(u,one,n)!=0 && MCL_FF_comp(v,one,n)!=0)
	{
		while (MCL_FF_parity(u)==0)
		{
			MCL_FF_shr(u,n);
			if (MCL_FF_parity(x1)!=0)
			{
				MCL_FF_add(x1,p,x1,n);
				MCL_FF_norm(x1,n);
			}
			MCL_FF_shr(x1,n);
		}
		while (MCL_FF_parity(v)==0)
		{
			MCL_FF_shr(v,n);
			if (MCL_FF_parity(x2)!=0)
			{
				MCL_FF_add(x2,p,x2,n);
				MCL_FF_norm(x2,n);
			}
			MCL_FF_shr(x2,n);
		}
		if (MCL_FF_comp(u,v,n)>=0)
		{

			MCL_FF_sub(u,u,v,n);
			MCL_FF_norm(u,n);
			if (MCL_FF_comp(x1,x2,n)>=0) MCL_FF_sub(x1,x1,x2,n);
			else
			{
				MCL_FF_sub(t,p,x2,n);
				MCL_FF_add(x1,x1,t,n);
			}
			MCL_FF_norm(x1,n);
		}
		else
		{
			MCL_FF_sub(v,v,u,n);
			MCL_FF_norm(v,n);
			if (MCL_FF_comp(x2,x1,n)>=0) MCL_FF_sub(x2,x2,x1,n);
			else
			{
				MCL_FF_sub(t,p,x1,n);
				MCL_FF_add(x2,x2,t,n);
			}
			MCL_FF_norm(x2,n);
		}
	}
	if (MCL_FF_comp(u,one,n)==0)
		MCL_FF_copy(r,x1,n);
	else
		MCL_FF_copy(r,x2,n);
}

/* nesidue mod m */
static void FF_nres(mcl_chunk a[][MCL_BS],mcl_chunk m[][MCL_BS],int n)
{
#ifndef C99
	mcl_chunk d[2*MCL_FFLEN][MCL_BS];
#else
	mcl_chunk d[2*n][MCL_BS];
#endif

	FF_dsucopy(d,a,n);
	MCL_FF_dmod(a,d,m,n);
}

static void FF_redc(mcl_chunk a[][MCL_BS],mcl_chunk m[][MCL_BS],mcl_chunk ND[][MCL_BS],int n)
{
#ifndef C99
	mcl_chunk d[2*MCL_FFLEN][MCL_BS];
#else
	mcl_chunk d[2*n][MCL_BS];
#endif
	MCL_FF_mod(a,m,n);
	FF_dscopy(d,a,n);
	FF_reduce(a,d,m,ND,n);
	MCL_FF_mod(a,m,n);
}

/* U=1/a mod 2^m - Arazi & Qi */
static void FF_invmod2m(mcl_chunk U[][MCL_BS],mcl_chunk a[][MCL_BS],int n)
{
	int i;
#ifndef C99
	mcl_chunk t1[MCL_FFLEN][MCL_BS],b[MCL_FFLEN][MCL_BS],c[MCL_FFLEN][MCL_BS];
#else
	mcl_chunk t1[n][MCL_BS],b[n][MCL_BS],c[n][MCL_BS];
#endif
	MCL_FF_zero(U,n);
	MCL_BIG_copy(U[0],a[0]);
	MCL_BIG_invmod2m(U[0]);

	for (i=1;i<n;i<<=1)
	{
		MCL_FF_copy(b,a,i);
		MCL_FF_mul(t1,U,b,i); MCL_FF_shrw(t1,i); // top half to bottom half, top half=0

		MCL_FF_copy(c,a,2*i); MCL_FF_shrw(c,i); // top half of c
		FF_lmul(b,U,c,i); // should set top half of b=0
		MCL_FF_add(t1,t1,b,i);  MCL_FF_norm(t1,2*i);
		FF_lmul(b,t1,U,i); MCL_FF_copy(t1,b,i);
		MCL_FF_one(b,i); MCL_FF_shlw(b,i);
		MCL_FF_sub(t1,b,t1,2*i); MCL_FF_norm(t1,2*i);
		MCL_FF_shlw(t1,i);
		MCL_FF_add(U,U,t1,2*i);
	}
	MCL_FF_norm(U,n);
}

void MCL_FF_random(mcl_chunk x[][MCL_BS],csprng *rng,int n)
{
	int i;
	for (i=0;i<n;i++)
	{
		MCL_BIG_random(x[i],rng);
	}
/* make sure top bit is 1 */
	while (MCL_BIG_nbits(x[n-1])<MCL_MODBYTES*8) MCL_BIG_random(x[n-1],rng);
}

/* generate random x mod p */
void MCL_FF_randomnum(mcl_chunk x[][MCL_BS],mcl_chunk p[][MCL_BS],csprng *rng,int n)
{
	int i;
#ifndef C99
	mcl_chunk d[2*MCL_FFLEN][MCL_BS];
#else
	mcl_chunk d[2*n][MCL_BS];
#endif
	for (i=0;i<2*n;i++)
	{
		MCL_BIG_random(d[i],rng);
	}
	MCL_FF_dmod(x,d,p,n);
}

static void MCL_FF_modmul(mcl_chunk z[][MCL_BS],mcl_chunk x[][MCL_BS],mcl_chunk y[][MCL_BS],mcl_chunk p[][MCL_BS],mcl_chunk ND[][MCL_BS],int n)
{
#ifndef C99
	mcl_chunk d[2*MCL_FFLEN][MCL_BS];
#else
	mcl_chunk d[2*n][MCL_BS];
#endif
	mcl_chunk ex=P_EXCESS(x[n-1]);
	mcl_chunk ey=P_EXCESS(y[n-1]);
	if ((ex+1)*(ey+1)+1>=P_FEXCESS) 
	{
#ifdef MCL_DEBUG_REDUCE
		printf("Product too large - reducing it %d %d\n",ex,ey);
#endif
		MCL_FF_mod(x,p,n); 
	}
	MCL_FF_mul(d,x,y,n);
	FF_reduce(z,d,p,ND,n);
}

static void MCL_FF_modsqr(mcl_chunk z[][MCL_BS],mcl_chunk x[][MCL_BS],mcl_chunk p[][MCL_BS],mcl_chunk ND[][MCL_BS],int n)
{
#ifndef C99
	mcl_chunk d[2*MCL_FFLEN][MCL_BS];
#else
	mcl_chunk d[2*n][MCL_BS];
#endif
	mcl_chunk ex=P_EXCESS(x[n-1]);
	if ((ex+1)*(ex+1)+1>=P_FEXCESS) 
	{
#ifdef MCL_DEBUG_REDUCE
		printf("Product too large - reducing it %d\n",ex);
#endif
		MCL_FF_mod(x,p,n); 
	}
	MCL_FF_sqr(d,x,n);
	FF_reduce(z,d,p,ND,n);
}

/* r=x^e mod p using side-channel resistant Montgomery Ladder, for large e */
void MCL_FF_skpow(mcl_chunk r[][MCL_BS],mcl_chunk x[][MCL_BS],mcl_chunk e[][MCL_BS],mcl_chunk p[][MCL_BS],int n)
{
	int i,b;
#ifndef C99
	mcl_chunk R0[MCL_FFLEN][MCL_BS],R1[MCL_FFLEN][MCL_BS],ND[MCL_FFLEN][MCL_BS];
#else
	mcl_chunk R0[n][MCL_BS],R1[n][MCL_BS],ND[n][MCL_BS];
#endif
	FF_invmod2m(ND,p,n);	

	MCL_FF_one(R0,n);
	MCL_FF_copy(R1,x,n);
	FF_nres(R0,p,n);
	FF_nres(R1,p,n);

	for (i=8*MCL_MODBYTES*n-1;i>=0;i--)
	{
		b=MCL_BIG_bit(e[i/MCL_BIGBITS],i%MCL_BIGBITS);
		MCL_FF_modmul(r,R0,R1,p,ND,n);

		FF_cswap(R0,R1,b,n);
		MCL_FF_modsqr(R0,R0,p,ND,n);

		MCL_FF_copy(R1,r,n);
		FF_cswap(R0,R1,b,n);
	}
	MCL_FF_copy(r,R0,n);
	FF_redc(r,p,ND,n);
}

/* r=x^e mod p using side-channel resistant Montgomery Ladder, for short e */
void MCL_FF_skspow(mcl_chunk r[][MCL_BS],mcl_chunk x[][MCL_BS],MCL_BIG e,mcl_chunk p[][MCL_BS],int n)
{
	int i,b;
#ifndef C99
	mcl_chunk R0[MCL_FFLEN][MCL_BS],R1[MCL_FFLEN][MCL_BS],ND[MCL_FFLEN][MCL_BS];
#else
	mcl_chunk R0[n][MCL_BS],R1[n][MCL_BS],ND[n][MCL_BS];
#endif
	FF_invmod2m(ND,p,n);
	MCL_FF_one(R0,n);
	MCL_FF_copy(R1,x,n);
	FF_nres(R0,p,n);
	FF_nres(R1,p,n);
	for (i=8*MCL_MODBYTES-1;i>=0;i--)
	{
		b=MCL_BIG_bit(e,i);
		MCL_FF_modmul(r,R0,R1,p,ND,n);
		FF_cswap(R0,R1,b,n);
		MCL_FF_modsqr(R0,R0,p,ND,n);
		MCL_FF_copy(R1,r,n);
		FF_cswap(R0,R1,b,n);
	}
	MCL_FF_copy(r,R0,n);
	FF_redc(r,p,ND,n);
}

/* raise to an integer power - right-to-left method */
void MCL_FF_power(mcl_chunk r[][MCL_BS],mcl_chunk x[][MCL_BS],int e,mcl_chunk p[][MCL_BS],int n)
{
	int f=1;
#ifndef C99
	mcl_chunk w[MCL_FFLEN][MCL_BS],ND[MCL_FFLEN][MCL_BS];
#else
	mcl_chunk w[n][MCL_BS],ND[n][MCL_BS];
#endif
	FF_invmod2m(ND,p,n);

	MCL_FF_copy(w,x,n);
	FF_nres(w,p,n);

	if (e==2)
	{
		MCL_FF_modsqr(r,w,p,ND,n);
	}
	else for (;;)
	{
		if (e%2==1) 
		{
			if (f) MCL_FF_copy(r,w,n);
			else MCL_FF_modmul(r,r,w,p,ND,n);
			f=0;
		}
		e>>=1;
		if (e==0) break;
		MCL_FF_modsqr(w,w,p,ND,n);
	}

	FF_redc(r,p,ND,n);
}

/* r=x^e mod p, faster but not side channel resistant */
void MCL_FF_pow(mcl_chunk r[][MCL_BS],mcl_chunk x[][MCL_BS],mcl_chunk e[][MCL_BS],mcl_chunk p[][MCL_BS],int n)
{
	int i,b;
#ifndef C99
	mcl_chunk w[MCL_FFLEN][MCL_BS],ND[MCL_FFLEN][MCL_BS];
#else
	mcl_chunk w[n][MCL_BS],ND[n][MCL_BS];
#endif
	FF_invmod2m(ND,p,n);
	MCL_FF_copy(w,x,n);
	MCL_FF_one(r,n);
	FF_nres(r,p,n);
	FF_nres(w,p,n);

	for (i=8*MCL_MODBYTES*n-1;i>=0;i--)
	{
		MCL_FF_modsqr(r,r,p,ND,n);
		b=MCL_BIG_bit(e[i/MCL_BIGBITS],i%MCL_BIGBITS);
		if (b==1) MCL_FF_modmul(r,r,w,p,ND,n);
	}
	FF_redc(r,p,ND,n);
}

/* double exponentiation r=x^e.y^f mod p */
void MCL_FF_pow2(mcl_chunk r[][MCL_BS],mcl_chunk x[][MCL_BS],MCL_BIG e,mcl_chunk y[][MCL_BS],MCL_BIG f,mcl_chunk p[][MCL_BS],int n)
{
	int i,eb,fb;
#ifndef C99
	mcl_chunk xn[MCL_FFLEN][MCL_BS],yn[MCL_FFLEN][MCL_BS],xy[MCL_FFLEN][MCL_BS],ND[MCL_FFLEN][MCL_BS];
#else
	mcl_chunk xn[n][MCL_BS],yn[n][MCL_BS],xy[n][MCL_BS],ND[n][MCL_BS];
#endif
	FF_invmod2m(ND,p,n);
	MCL_FF_copy(xn,x,n);
	MCL_FF_copy(yn,y,n);
	FF_nres(xn,p,n);
	FF_nres(yn,p,n);
	MCL_FF_modmul(xy,xn,yn,p,ND,n);
	MCL_FF_one(r,n);
	FF_nres(r,p,n);

	for (i=8*MCL_MODBYTES-1;i>=0;i--)
	{
		eb=MCL_BIG_bit(e,i);
		fb=MCL_BIG_bit(f,i);
		MCL_FF_modsqr(r,r,p,ND,n);
		if (eb==1)
		{
			if (fb==1) MCL_FF_modmul(r,r,xy,p,ND,n);
			else MCL_FF_modmul(r,r,xn,p,ND,n);
		}
		else
		{
			if (fb==1) MCL_FF_modmul(r,r,yn,p,ND,n);
		}
	}
	FF_redc(r,p,ND,n);
}

static sign32 igcd(sign32 x,sign32 y)
{ /* integer GCD, returns GCD of x and y */
    sign32 r;
    if (y==0) return x;
    while ((r=x%y)!=0)
        x=y,y=r;
    return y;
}

/* quick and dirty check for common factor with s */
int MCL_FF_cfactor(mcl_chunk w[][MCL_BS],sign32 s,int n)
{
	int r;
	sign32 g;
#ifndef C99
	mcl_chunk x[MCL_FFLEN][MCL_BS],y[MCL_FFLEN][MCL_BS];
#else
	mcl_chunk x[n][MCL_BS],y[n][MCL_BS];
#endif
	MCL_FF_init(y,s,n);
	MCL_FF_copy(x,w,n);
	MCL_FF_norm(x,n);
	
//	if (MCL_FF_parity(x)==0) return 1;
	do
	{
		MCL_FF_sub(x,x,y,n);
		MCL_FF_norm(x,n);
		while (!MCL_FF_iszilch(x,n) && MCL_FF_parity(x)==0) MCL_FF_shr(x,n);
	}
	while (MCL_FF_comp(x,y,n)>0);
#if MCL_CHUNK<32
	g=x[0][0]+((sign32)(x[0][1])<<MCL_BASEBITS);
#else
	g=(sign32)x[0][0];
#endif
	r=igcd(s,g);
//printf("r= %d\n",r);
	if (r>1) return 1;
	return 0;
}

/* Miller-Rabin test for primality. Slow. */
int MCL_FF_prime(mcl_chunk p[][MCL_BS],csprng *rng,int n)
{
	int i,j,loop,s=0;
#ifndef C99
	mcl_chunk d[MCL_FFLEN][MCL_BS],x[MCL_FFLEN][MCL_BS],unity[MCL_FFLEN][MCL_BS],nm1[MCL_FFLEN][MCL_BS];
#else
	mcl_chunk d[n][MCL_BS],x[n][MCL_BS],unity[n][MCL_BS],nm1[n][MCL_BS];
#endif
	sign32 sf=4849845;/* 3*5*.. *19 */

	MCL_FF_norm(p,n);
	if (MCL_FF_cfactor(p,sf,n)) return 0;

	MCL_FF_one(unity,n);
	MCL_FF_sub(nm1,p,unity,n);
	MCL_FF_norm(nm1,n);
	MCL_FF_copy(d,nm1,n);

	while (MCL_FF_parity(d)==0)
	{
		MCL_FF_shr(d,n);
		s++;
	}
	if (s==0) return 0;

	for (i=0;i<10;i++)
	{
		MCL_FF_randomnum(x,p,rng,n);
		MCL_FF_pow(x,x,d,p,n);

		if (MCL_FF_comp(x,unity,n)==0 || MCL_FF_comp(x,nm1,n)==0) continue;
		loop=0;
		for (j=1;j<s;j++)
		{
			MCL_FF_power(x,x,2,p,n);
			if (MCL_FF_comp(x,unity,n)==0) return 0;
			if (MCL_FF_comp(x,nm1,n)==0 ) {loop=1; break;}
		}
		if (loop) continue;
		return 0;
	}

	return 1;
}

/*
MCL_BIG P[4]= {{0x1670957,0x1568CD3C,0x2595E5,0xEED4F38,0x1FC9A971,0x14EF7E62,0xA503883,0x9E1E05E,0xBF59E3},{0x1844C908,0x1B44A798,0x3A0B1E7,0xD1B5B4E,0x1836046F,0x87E94F9,0x1D34C537,0xF7183B0,0x46D07},{0x17813331,0x19E28A90,0x1473A4D6,0x1CACD01F,0x1EEA8838,0xAF2AE29,0x1F85292A,0x1632585E,0xD945E5},{0x919F5EF,0x1567B39F,0x19F6AD11,0x16CE47CF,0x9B36EB1,0x35B7D3,0x483B28C,0xCBEFA27,0xB5FC21}};

int main()
{
	int i;
	MCL_BIG p[4],e[4],x[4],r[4];
	csprng rng;
	char raw[100];
	for (i=0;i<100;i++) raw[i]=i;
    MCL_RAND_seed(&rng,100,raw);


	MCL_FF_init(x,3,4);

	MCL_FF_copy(p,P,4);
	MCL_FF_copy(e,p,4);
	MCL_FF_dec(e,1,4);
	MCL_FF_norm(e,4);



	printf("p= ");MCL_FF_output(p,4); printf("\n");
	if (MCL_FF_prime(p,&rng,4)) printf("p is a prime\n");
	printf("e= ");MCL_FF_output(e,4); printf("\n");

	MCL_FF_skpow(r,x,e,p,4);
	printf("r= ");MCL_FF_output(r,4); printf("\n");
}

*/
