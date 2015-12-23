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


/* ARAcrypt mod p functions */
/* Small Finite Field arithmetic */
/* SU=m, SU is Stack Usage (MCL_NOT_SPECIAL MCL_Modulus) */

#include "mcl_arch.h"
#include "mcl_config.h"
#include "mcl_big.h"
#include "mcl_fp.h"

#define MCL_NLEN (1+((MCL_MBITS-1)/MCL_BASEBITS))	/**< Number of words in MCL_BIG. */
#define DMCL_NLEN 2*MCL_NLEN	/**< double length required for products of MCL_BIGs */

#ifdef MCL_DEBUG_NORM
#define DMCL_BS (2*MCL_BS-1)
#else
#define DMCL_BS (2*MCL_BS)
#endif

#define BMASK (((mcl_chunk)1<<MCL_BASEBITS)-1) /**< Mask = 2^MCL_BASEBITS-1 */

#define MB (MCL_MBITS%MCL_BASEBITS) /**<  Number of bits in modulus mod number of bits in number base */
#define TBITS (MCL_MBITS%MCL_BASEBITS) /**< Number of active bits in top word */
#define TMASK (((mcl_chunk)1<<(MCL_MBITS%MCL_BASEBITS))-1)  /**< Mask for active bits in top word */
#define NEXCESS (1<<(MCL_CHUNK-MCL_BASEBITS-1)) /**< 2^(MCL_CHUNK-MCL_BASEBITS-1) - digit cannot be multiplied by more than this before normalisation */
#define FEXCESS ((mcl_chunk)1<<(MCL_BASEBITS*MCL_NLEN-MCL_MBITS)) /**< 2^(MCL_BASEBITS*MCL_NLEN-MODBITS) - normalised MCL_BIG can be multiplied by more than this before reduction */
#define OMASK ((mcl_chunk)(-1)<<(MCL_MBITS%MCL_BASEBITS))     /**<  for masking out overflow bits */

/* catch field excesses */
#define EXCESS(a) ((a[MCL_NLEN-1]&OMASK)>>(MB))   /**< Field Excess */


/* Fast Modular Reduction Methods */

/* r=d mod m */
/* d MUST be normalised */
/* Products must be less than pR in all cases !!! */
/* So when multiplying two numbers, their product *must* be less than MCL_MBITS+MCL_BASEBITS*MCL_NLEN */
/* Results *may* be one bit bigger than MCL_MBITS */

#if MCL_MODTYPE == MCL_PSEUDO_MERSENNE
/* r=d mod m */

void MCL_FP_nres(MCL_BIG a) {}

void MCL_FP_redc(MCL_BIG a) {}

/* reduce a DMCL_BIG to a MCL_BIG exploiting the special form of the modulus */
void MCL_FP10_mod(MCL_BIG r,DMCL_BIG d)
{
	mcl_chunk t[MCL_BS],b[MCL_BS];
	mcl_chunk v,tw;
	MCL_BIG_split(t,b,d,MCL_MBITS); 

/* Note that all of the excess gets pushed into t. So if squaring a value with a 4-bit excess, this results in
   t getting all 8 bits of the excess product! So products must be less than pR which is Montgomery compatible */

	if (MCL_MConst < NEXCESS)
	{
		MCL_BIG_imul(t,t,MCL_MConst);

		MCL_BIG_norm(t);
		tw=t[MCL_NLEN-1]; 
		t[MCL_NLEN-1]&=TMASK;
		t[0]+=MCL_MConst*((tw>>TBITS));  
	}
	else
	{
		v=MCL_BIG_pmul(t,t,MCL_MConst);
		tw=t[MCL_NLEN-1]; 
		t[MCL_NLEN-1]&=TMASK;
#if MCL_CHUNK == 16
		t[1]+=MCL_muladd(MCL_MConst,((tw>>TBITS)+(v<<(MCL_BASEBITS-TBITS))),0,&t[0]);
#else
		t[0]+=MCL_MConst*((tw>>TBITS)+(v<<(MCL_BASEBITS-TBITS)));
#endif
	}
	MCL_BIG_add(r,t,b);
	MCL_BIG_norm(r);
}
#endif

/* This only applies to Curve MCL_C448, so specialised (for now) */
#if MCL_MODTYPE == MCL_GENERALISED_MERSENNE

void MCL_FP_nres(MCL_BIG a) {}

void MCL_FP_redc(MCL_BIG a) {}

/* reduce a DMCL_BIG to a MCL_BIG exploiting the special form of the modulus */
void MCL_FP10_mod(MCL_BIG r,DMCL_BIG d)
{
	mcl_chunk t[MCL_BS],b[MCL_BS];

	MCL_BIG_split(t,b,d,MCL_MBITS); 

	MCL_BIG_add(r,t,b);
	MCL_BIG_dscopy(d,t);
	MCL_BIG_dshl(d,MCL_MBITS/2);

	MCL_BIG_split(t,b,d,MCL_MBITS);
	MCL_BIG_add(r,r,t);
	MCL_BIG_add(r,r,b);
	MCL_BIG_shl(t,MCL_MBITS/2);

	MCL_BIG_add(r,r,t);

	MCL_BIG_norm(r);
}

#endif

#if MCL_MODTYPE == MCL_MCL_MONTGOMERY_FRIENDLY

/* convert to Montgomery n-residue form */
void MCL_FP_nres(MCL_BIG a)
{
	mcl_chunk d[DMCL_BS];
	mcl_chunk m[MCL_BS];
	MCL_BIG_rcopy(m,MCL_Modulus);
	MCL_BIG_dscopy(d,a);
	MCL_BIG_dshl(d,MCL_NLEN*MCL_BASEBITS);
	MCL_BIG_dmod(a,d,m);
}

/* convert back to regular form */
void MCL_FP_redc(MCL_BIG a)
{
	mcl_chunk d[DMCL_BS];
	MCL_BIG_dzero(d);
	MCL_BIG_dscopy(d,a);
	MCL_FP10_mod(a,d);
}

/* fast modular reduction from DMCL_BIG to MCL_BIG exploiting special form of the modulus */
void MCL_FP10_mod(MCL_BIG a,DMCL_BIG d)
{	
	int i;
	mcl_chunk k;

	for (i=0;i<MCL_NLEN;i++)
		d[MCL_NLEN+i]+=MCL_muladd(d[i],MCL_MConst-1,d[i],&d[MCL_NLEN+i-1]);

	MCL_BIG_sducopy(a,d);
	MCL_BIG_norm(a);
}

#endif

#if MCL_MODTYPE == MCL_NOT_SPECIAL

/* convert MCL_BIG a to Montgomery n-residue form */
/* SU= 120 */
void MCL_FP_nres(MCL_BIG a)
{
	mcl_chunk d[DMCL_BS];
	mcl_chunk m[MCL_BS];
	MCL_BIG_rcopy(m,MCL_Modulus);
	MCL_BIG_dscopy(d,a);
	MCL_BIG_dshl(d,MCL_NLEN*MCL_BASEBITS);
	MCL_BIG_dmod(a,d,m);
}

/* SU= 80 */
/* convert back to regular form */
void MCL_FP_redc(MCL_BIG a)
{
	mcl_chunk d[DMCL_BS];
	MCL_BIG_dzero(d);
	MCL_BIG_dscopy(d,a);
	MCL_FP10_mod(a,d);
}

/* reduce a DMCL_BIG to a MCL_BIG using Montgomery's no trial division method */
/* d is expected to be dnormed before entry */
/* SU= 112 */
void MCL_FP10_mod(MCL_BIG a,DMCL_BIG d)
{
	int i,j;
	mcl_chunk md[MCL_BS];

#ifdef mcl_dchunk
	mcl_dchunk sum;
	mcl_chunk sp;
#endif

	MCL_BIG_rcopy(md,MCL_Modulus);

#ifdef MCL_COMBA

/* Faster to Combafy it.. Let the compiler unroll the loops! */

	sum=d[0];
	for (j=0;j<MCL_NLEN;j++)
	{
		for (i=0;i<j;i++) sum+=(mcl_dchunk)d[i]*md[j-i];
		if (MCL_MConst==-1) sp=(-(mcl_chunk)sum)&BMASK;
		else
		{
			if (MCL_MConst==1) sp=((mcl_chunk)sum)&BMASK;
			else sp=((mcl_chunk)sum*MCL_MConst)&BMASK;
		}
		d[j]=sp; sum+=(mcl_dchunk)sp*md[0];  /* no need for &BMASK here! */
		sum=d[j+1]+(sum>>MCL_BASEBITS);
	}

	for (j=MCL_NLEN;j<DMCL_NLEN-2;j++)
	{
		for (i=j-MCL_NLEN+1;i<MCL_NLEN;i++) sum+=(mcl_dchunk)d[i]*md[j-i];
		d[j]=(mcl_chunk)sum&BMASK;
		sum=d[j+1]+(sum>>MCL_BASEBITS); 
	}

	sum+=(mcl_dchunk)d[MCL_NLEN-1]*md[MCL_NLEN-1];
	d[DMCL_NLEN-2]=(mcl_chunk)sum&BMASK;
	sum=d[DMCL_NLEN-1]+(sum>>MCL_BASEBITS); 
	d[DMCL_NLEN-1]=(mcl_chunk)sum&BMASK;

	MCL_BIG_sducopy(a,d);
	MCL_BIG_norm(a);

#else
	mcl_chunk m,carry;
	for (i=0;i<MCL_NLEN;i++) 
	{
		if (MCL_MConst==-1) m=(-d[i])&BMASK;
		else
		{
			if (MCL_MConst==1) m=d[i];
			else m=(MCL_MConst*d[i])&BMASK;
		}
		carry=0;
		for (j=0;j<MCL_NLEN;j++)
			carry=MCL_muladd(m,md[j],carry,&d[i+j]);
		d[MCL_NLEN+i]+=carry;
	}
	MCL_BIG_sducopy(a,d);
	MCL_BIG_norm(a);

#endif
}

#endif

/* test x==0 ? */
/* SU= 48 */
int MCL_FP_iszilch(MCL_BIG x)
{
	mcl_chunk m[MCL_BS];
	MCL_BIG_rcopy(m,MCL_Modulus);
	MCL_BIG_mod(x,m);
	return MCL_BIG_iszilch(x);
}

/* output FP */
/* SU= 48 */
void MCL_FP_output(MCL_BIG r)
{
	mcl_chunk c[MCL_BS];
	MCL_BIG_copy(c,r);
	MCL_FP_redc(c);
	MCL_BIG_output(c);
}

void MCL_FP_rawoutput(MCL_BIG r)
{
	MCL_BIG_rawoutput(r);
}

#ifdef MCL_GET_STATS
int tsqr=0,rsqr=0,tmul=0,rmul=0;
int tadd=0,radd=0,tneg=0,rneg=0;
int tdadd=0,rdadd=0,tdneg=0,rdneg=0;
#endif

/* r=a*b mod MCL_Modulus */ 
/* product must be less that p.R - and we need to know this in advance! */
/* SU= 88 */
void MCL_FP_mul(MCL_BIG r,MCL_BIG a,MCL_BIG b)
{
	mcl_chunk d[DMCL_BS];
	mcl_chunk ea=EXCESS(a);
	mcl_chunk eb=EXCESS(b);
	if ((ea+1)*(eb+1)+1>=FEXCESS) 
	{
#ifdef MCL_DEBUG_REDUCE
		printf("Product too large - reducing it %d %d\n",ea,eb);
#endif
		MCL_FP_reduce(a);  /* it is sufficient to fully reduce just one of them < p */
#ifdef MCL_GET_STATS
		rmul++;
	}
	tmul++;
#else
	}
#endif

	MCL_BIG_mul(d,a,b);
	MCL_FP10_mod(r,d);
}

/* multiplication by an integer, r=a*c */
/* SU= 136 */
void MCL_FP_imul(MCL_BIG r,MCL_BIG a,int c)
{
	mcl_chunk d[DMCL_BS];
	mcl_chunk m[MCL_BS];
	int s=0;
	mcl_chunk afx;
	MCL_BIG_norm(a);
	if (c<0)
	{
		c=-c;
		s=1;
	}
	afx=(EXCESS(a)+1)*(c+1)+1;
	if (c<NEXCESS && afx<FEXCESS)
		MCL_BIG_imul(r,a,c);
	else 
	{
		if (afx<FEXCESS)
		{
			MCL_BIG_pmul(r,a,c);
		}
		else
		{
			MCL_BIG_rcopy(m,MCL_Modulus);	
			MCL_BIG_pxmul(d,a,c);
			MCL_BIG_dmod(r,d,m);
		}
	}
	if (s) MCL_FP_neg(r,r);
	MCL_BIG_norm(r);
}

/* Set r=a^2 mod m */ 
/* SU= 88 */
void MCL_FP_sqr(MCL_BIG r,MCL_BIG a)
{
	mcl_chunk d[DMCL_BS];
	mcl_chunk ea=EXCESS(a);
	if ((ea+1)*(ea+1)+1>=FEXCESS) 
	{
#ifdef MCL_DEBUG_REDUCE
		printf("Product too large - reducing it %d\n",ea);
#endif
		MCL_FP_reduce(a);
#ifdef MCL_GET_STATS
		rsqr++;
	}
	tsqr++;
#else
	}
#endif
	MCL_BIG_sqr(d,a); 
	MCL_FP10_mod(r,d);
}

/* SU= 16 */
/* Set r=a+b */
void MCL_FP_add(MCL_BIG r,MCL_BIG a,MCL_BIG b)
{
	MCL_BIG_add(r,a,b);
	if (EXCESS(r)+2>=FEXCESS)  /* +2 because a and b not normalised */
	{
#ifdef MCL_DEBUG_REDUCE
		printf("Sum too large - reducing it %d\n",EXCESS(r));
#endif
		MCL_FP_reduce(r);
#ifdef MCL_GET_STATS
		radd++;
	}
	tadd++;
#else
	}
#endif
}

/* Set r=a-b mod m */
/* SU= 56 */
void MCL_FP_sub(MCL_BIG r,MCL_BIG a,MCL_BIG b)
{
	mcl_chunk n[MCL_BS];
	MCL_FP_neg(n,b);
	MCL_FP_add(r,a,n);
}

/* SU= 48 */
/* Fully reduce a mod MCL_Modulus */
void MCL_FP_reduce(MCL_BIG a)
{
	mcl_chunk m[MCL_BS];
	MCL_BIG_rcopy(m,MCL_Modulus);
	MCL_BIG_mod(a,m);
}

/* Set r=-a mod MCL_Modulus */
/* SU= 64 */
void MCL_FP_neg(MCL_BIG r,MCL_BIG a)
{
	int sb;
	mcl_chunk ov;
	mcl_chunk m[MCL_BS];

	MCL_BIG_rcopy(m,MCL_Modulus);
	MCL_BIG_norm(a);

	ov=EXCESS(a); 
	sb=1; while(ov!=0) {sb++;ov>>=1;}  /* only unpredictable branch */

	MCL_BIG_fshl(m,sb);
	MCL_BIG_sub(r,m,a);

	if (EXCESS(r)>=FEXCESS)
	{
#ifdef MCL_DEBUG_REDUCE
		printf("Negation too large -  reducing it %d\n",EXCESS(r));
#endif
		MCL_FP_reduce(r);
#ifdef MCL_GET_STATS
		rneg++;
	}
	tneg++;
#else
	}
#endif

}

/* Set r=a/2. */
/* SU= 56 */
void MCL_FP_div2(MCL_BIG r,MCL_BIG a)
{
	mcl_chunk m[MCL_BS];
	MCL_BIG_rcopy(m,MCL_Modulus);
	MCL_BIG_norm(a);
	if (MCL_BIG_parity(a)==0)
	{
		MCL_BIG_copy(r,a);
		MCL_BIG_fshr(r,1);
	}
	else
	{
		MCL_BIG_add(r,a,m);
		MCL_BIG_norm(r);
		MCL_BIG_fshr(r,1);
	}
}

/* set w=1/x */
void MCL_FP_inv(MCL_BIG w,MCL_BIG x)
{
	mcl_chunk m[MCL_BS];
	MCL_BIG_rcopy(m,MCL_Modulus);
	MCL_BIG_copy(w,x);
	MCL_FP_redc(w);

	MCL_BIG_invmodp(w,w,m);
	MCL_FP_nres(w);
}

/* SU=8 */
/* set n=1 */
void MCL_FP_one(MCL_BIG n)
{
	MCL_BIG_one(n); MCL_FP_nres(n);
}

/* Set r=a^b mod MCL_Modulus */
/* SU= 136 */
void MCL_FP_pow(MCL_BIG r,MCL_BIG a,MCL_BIG b)
{
	mcl_chunk w[MCL_BS],z[MCL_BS],zilch[MCL_BS];
	int bt;
	MCL_BIG_zero(zilch);

	MCL_BIG_norm(b);
	MCL_BIG_copy(z,b);
	MCL_BIG_copy(w,a);
	MCL_FP_one(r);
	while(1)
	{
		bt=MCL_BIG_parity(z);
		MCL_BIG_fshr(z,1);
		if (bt) MCL_FP_mul(r,r,w);
		if (MCL_BIG_comp(z,zilch)==0) break;
		MCL_FP_sqr(w,w);
	}
	MCL_FP_reduce(r);
}

/* is r a QR? */
int MCL_FP_qr(MCL_BIG r)
{
	int j;
	mcl_chunk m[MCL_BS];
	MCL_BIG_rcopy(m,MCL_Modulus);
	MCL_FP_redc(r);
	j=MCL_BIG_jacobi(r,m);
	MCL_FP_nres(r);
	if (j==1) return 1;
	return 0;

}

/* Set a=sqrt(b) mod MCL_Modulus */
/* SU= 160 */
void MCL_FP_sqrt(MCL_BIG r,MCL_BIG a)
{
	mcl_chunk v[MCL_BS],i[MCL_BS],b[MCL_BS];
	mcl_chunk m[MCL_BS];
	int mod8;
	MCL_BIG_rcopy(m,MCL_Modulus);
	MCL_BIG_mod(a,m);
	MCL_BIG_copy(b,m);
	mod8=m[0]%8;
	if (mod8==5)
	{
		MCL_BIG_dec(b,5); MCL_BIG_norm(b); MCL_BIG_fshr(b,3); /* (p-5)/8 */
		MCL_BIG_copy(i,a); MCL_BIG_fshl(i,1);
		MCL_FP_pow(v,i,b);
		MCL_FP_mul(i,i,v); MCL_FP_mul(i,i,v);
		MCL_BIG_dec(i,1); 
		MCL_FP_mul(r,a,v); MCL_FP_mul(r,r,i);
		MCL_BIG_mod(r,m);
	}
	if (mod8==3 || mod8==7)
	{
		MCL_BIG_inc(b,1); MCL_BIG_norm(b); MCL_BIG_fshr(b,2); /* (p+1)/4 */
		MCL_FP_pow(r,a,b);
	}
}

/*
int main()
{

	MCL_BIG r,m,e;

	MCL_BIG_zero(m); MCL_BIG_inc(m,3);
	MCL_BIG_output(m);
	printf("\n");

	MCL_BIG_rcopy(r,MCL_Modulus);

	MCL_BIG_copy(e,r);
	MCL_BIG_dec(e,1);
	MCL_BIG_norm(e);

	MCL_FP_pow(r,m,e);

	MCL_BIG_output(r);
} */
/*
	int i,carry;
	DMCL_BIG c={0,0,0,0,0,0,0,0};
	MCL_BIG a={1,2,3,4};
	MCL_BIG b={3,4,5,6};
	MCL_BIG r={11,12,13,14};
	MCL_BIG s={23,24,25,15};
	MCL_BIG w;

//	printf("NEXCESS= %d\n",NEXCESS);
//	printf("MCL_MConst= %d\n",MCL_MConst);

	MCL_BIG_copy(b,MCL_Modulus);
	MCL_BIG_dec(b,1);
	MCL_BIG_norm(b);

	MCL_BIG_randomnum(r); MCL_BIG_norm(r); MCL_BIG_mod(r,MCL_Modulus);
//	MCL_BIG_randomnum(s); norm(s); MCL_BIG_mod(s,MCL_Modulus);

//	MCL_BIG_output(r);
//	MCL_BIG_output(s);

	MCL_BIG_output(r);
	MCL_FP_nres(r);
	MCL_BIG_output(r);
	MCL_BIG_copy(a,r);
	MCL_FP_redc(r);
	MCL_BIG_output(r);
	MCL_BIG_dscopy(c,a);
	MCL_FP10_mod(r,c);
	MCL_BIG_output(r);


//	exit(0);

//	copy(r,a);
	printf("r=   "); MCL_BIG_output(r);
	MCL_BIG_modsqr(r,r,MCL_Modulus);
	printf("r^2= "); MCL_BIG_output(r);

	MCL_FP_nres(r);
	MCL_FP_sqrt(r,r);
	MCL_FP_redc(r);
	printf("r=   "); MCL_BIG_output(r);
	MCL_BIG_modsqr(r,r,MCL_Modulus);
	printf("r^2= "); MCL_BIG_output(r);


//	for (i=0;i<100000;i++) MCL_FP_sqr(r,r);
//	for (i=0;i<100000;i++)
		MCL_FP_sqrt(r,r);

	MCL_BIG_output(r);
}
*/
