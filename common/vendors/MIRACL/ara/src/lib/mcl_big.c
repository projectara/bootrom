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


/* ARAcrypt basic functions for MCL_BIG type */
/* SU=m, SU is Stack Usage */

#include "mcl_arch.h"
#include "mcl_config.h"
#include "mcl_big.h"

#define MCL_MODBYTES (1+(MCL_MBITS-1)/8) /**< Number of bytes in MCL_Modulus */
#define MCL_BIGBITS (MCL_MODBYTES*8) /**< Number of bits representable in a MCL_BIG */

#ifdef MCL_DEBUG_NORM
#define DMCL_BS (2*MCL_BS-1)
#else
#define DMCL_BS (2*MCL_BS)
#endif

#define MCL_NLEN (1+((MCL_MBITS-1)/MCL_BASEBITS))	/**< Number of words in MCL_BIG. */
#define DMCL_CHUNK 2*MCL_CHUNK	/**< Number of bits in double-length type */
#define DMCL_NLEN 2*MCL_NLEN	/**< double length required for products of MCL_BIGs */

#define WEXCESS (MCL_CHUNK-MCL_BASEBITS-1) /**< Word Excess in bits */
#define MCL_HFLEN (MCL_FFLEN/2)		/**< Useful for half-size RSA private key operations */

#define HBITS (MCL_BASEBITS/2)  /**< Number of bits in number base divided by 2 */
#define HBITS1 ((MCL_BASEBITS+1)/2) /**< Number of bits in number base plus 1 divided by 2 */
#define HDIFF (HBITS1-HBITS)  /**< Will be either 0 or 1, depending if number of bits in number base is even or odd */

#define BMASK (((mcl_chunk)1<<MCL_BASEBITS)-1) /**< Mask = 2^MCL_BASEBITS-1 */
#define HMASK (((mcl_chunk)1<<HBITS)-1)   /**< Mask = 2^HBITS-1 */
#define HMASK1 (((mcl_chunk)1<<HBITS1)-1) /**< Mask = 2^HBITS1-1 */


#define MB (MCL_MBITS%MCL_BASEBITS) /**<  Number of bits in modulus mod number of bits in number base */
#define TBITS (MCL_MBITS%MCL_BASEBITS) /**< Number of active bits in top word */
#define TMASK (((mcl_chunk)1<<(MCL_MBITS%MCL_BASEBITS))-1)  /**< Mask for active bits in top word */
#define NEXCESS (1<<(MCL_CHUNK-MCL_BASEBITS-1)) /**< 2^(MCL_CHUNK-MCL_BASEBITS-1) - digit cannot be multiplied by more than this before normalisation */
#define FEXCESS ((mcl_chunk)1<<(MCL_BASEBITS*MCL_NLEN-MCL_MBITS)) /**< 2^(MCL_BASEBITS*MCL_NLEN-MODBITS) - normalised MCL_BIG can be multiplied by more than this before reduction */
#define OMASK ((mcl_chunk)(-1)<<(MCL_MBITS%MCL_BASEBITS))     /**<  for masking out overflow bits */

/* catch field excesses */
#define EXCESS(a) ((a[MCL_NLEN-1]&OMASK)>>(MB))   /**< Field Excess */


#define abs(x) ((x>=0)?x:-x)

/* Calculates x*y+c+*r */

#ifdef mcl_dchunk

/* Method required to calculate x*y+c+r, bottom half in r, top half returned */
mcl_chunk MCL_muladd(mcl_chunk x,mcl_chunk y,mcl_chunk c,mcl_chunk *r) 
{
	mcl_dchunk prod=(mcl_dchunk)x*y+c+*r;
	*r=(mcl_chunk)prod&BMASK;
	return (mcl_chunk)(prod>>MCL_BASEBITS);
}

#else

/* No integer type available that can store double the wordlength */
/* accumulate partial products */

mcl_chunk MCL_muladd(mcl_chunk x,mcl_chunk y,mcl_chunk c,mcl_chunk *r) 
{
	mcl_chunk x0,x1,y0,y1;
	mcl_chunk bot,top,mid,carry;
	x0=x&HMASK;
	x1=(x>>HBITS);
	y0=y&HMASK;
	y1=(y>>HBITS);
	bot=x0*y0;
	top=x1*y1;
	mid=x0*y1+x1*y0;
	x0=mid&HMASK1;
	x1=(mid>>HBITS1);
	bot+=x0<<HBITS; bot+=*r; bot+=c;

#if HDIFF==1
	bot+=(top&HDIFF)<<(MCL_BASEBITS-1); 
	top>>=HDIFF;
#endif

	top+=x1;
	carry=bot>>MCL_BASEBITS;
	bot&=BMASK;
	top+=carry;

	*r=bot;
	return top;
}

#endif

/* test a=0? */
int MCL_BIG_iszilch(MCL_BIG a)
{
	int i;
	for (i=0;i<MCL_NLEN;i++)
		if (a[i]!=0) return 0;
	return 1; 	
}

/* test a=0? */
int MCL_BIG_diszilch(DMCL_BIG a)
{
	int i;
	for (i=0;i<DMCL_NLEN;i++)
		if (a[i]!=0) return 0;
	return 1; 	
}

/* SU= 56 */
/* output a */
#ifdef MCL_BUILD_TEST
void MCL_BIG_output(MCL_BIG a)
{
	mcl_chunk b[MCL_BS];
	int i,len;
	len=MCL_BIG_nbits(a);
	if (len%4==0) len/=4;
	else {len/=4; len++;}
	if (len<MCL_MODBYTES*2) len=MCL_MODBYTES*2;

	for (i=len-1;i>=0;i--)
	{
		MCL_BIG_copy(b,a);
		MCL_BIG_shr(b,i*4);
		printf("%01x",(unsigned int) b[0]&15);
	}
}

/* SU= 16 */
void MCL_BIG_rawoutput(MCL_BIG a)
{
	int i;
	printf("(");
	for (i=0;i<MCL_NLEN-1;i++)
	  printf("%llx,",(long long unsigned int) a[i]);
	printf("%llx)",(long long unsigned int) a[MCL_NLEN-1]);
}
#endif

/* Swap a and b if d=1 */
void MCL_BIG_cswap(MCL_BIG a,MCL_BIG b,int d)
{
	int i;
	mcl_chunk t,c=d;
	c=~(c-1);
#ifdef MCL_DEBUG_NORM
	for (i=0;i<=MCL_NLEN;i++)
#else
	for (i=0;i<MCL_NLEN;i++)
#endif
	{
		t=c&(a[i]^b[i]);
		a[i]^=t;
		b[i]^=t;
	}
}

/* Move b to a if d=1 */
void MCL_BIG_cmove(MCL_BIG f,MCL_BIG g,int d)
{
	int i;
	mcl_chunk b=(mcl_chunk)-d;
#ifdef MCL_DEBUG_NORM
	for (i=0;i<=MCL_NLEN;i++)
#else
	for (i=0;i<MCL_NLEN;i++)
#endif
	{
		f[i]^=(f[i]^g[i])&b;
	}
}

/* convert MCL_BIG to/from bytes */
/* SU= 64 */
void MCL_BIG_toBytes(char *b,MCL_BIG a)
{
	int i;
	mcl_chunk c[MCL_BS];
	MCL_BIG_norm(a);
	MCL_BIG_copy(c,a);
	for (i=MCL_MODBYTES-1;i>=0;i--)
	{
		b[i]=c[0]&0xff;
		MCL_BIG_fshr(c,8);
	}
}

/* SU= 16 */
void MCL_BIG_fromBytes(MCL_BIG a,char *b)
{
	int i;
	MCL_BIG_zero(a);
	for (i=0;i<MCL_MODBYTES;i++)
	{
		MCL_BIG_fshl(a,8); a[0]+=(int)(unsigned char)b[i];
		//MCL_BIG_inc(a,(int)(unsigned char)b[i]); MCL_BIG_norm(a);
	}
#ifdef MCL_DEBUG_NORM
	a[MCL_NLEN]=0;
#endif
}

void MCL_BIG_fromBytesLen(MCL_BIG a,char *b,int s)
{
	int i,len=s;
	MCL_BIG_zero(a);

	if (s>MCL_MODBYTES) s=MCL_MODBYTES;
	for (i=0;i<len;i++)
	{
		MCL_BIG_fshl(a,8); a[0]+=(int)(unsigned char)b[i];
	}
#ifdef MCL_DEBUG_NORM
	a[MCL_NLEN]=0;
#endif
}

#ifdef MCL_BUILD_TEST
/* SU= 88 */
void MCL_BIG_doutput(DMCL_BIG a)
{
	mcl_chunk b[DMCL_BS];
	int i,len;
	MCL_BIG_dnorm(a);
	len=MCL_BIG_dnbits(a);
	if (len%4==0) len/=4;
	else {len/=4; len++;}

	for (i=len-1;i>=0;i--)
	{
		MCL_BIG_dcopy(b,a);
		MCL_BIG_dshr(b,i*4);
		printf("%01x",(unsigned int) b[0]&15);
	}
}
#endif

/* Copy b=a */
void MCL_BIG_copy(MCL_BIG b,MCL_BIG a)
{
	int i;
	for (i=0;i<MCL_NLEN;i++)
		b[i]=a[i];
#ifdef MCL_DEBUG_NORM
	b[MCL_NLEN]=a[MCL_NLEN];
#endif
}

/* Copy from ROM b=a */
void MCL_BIG_rcopy(MCL_BIG b,const MCL_BIG a)
{
	int i;
	for (i=0;i<MCL_NLEN;i++)
		b[i]=a[i];
#ifdef MCL_DEBUG_NORM
	b[MCL_NLEN]=0;
#endif
}

/* double length DMCL_BIG copy b=a */
void MCL_BIG_dcopy(DMCL_BIG b,DMCL_BIG a)
{
	int i;
	for (i=0;i<DMCL_NLEN;i++)
		b[i]=a[i];
#ifdef MCL_DEBUG_NORM
	b[DMCL_NLEN]=a[DMCL_NLEN];
#endif
}

/* Copy MCL_BIG to bottom half of DMCL_BIG */
void MCL_BIG_dscopy(DMCL_BIG b,MCL_BIG a)
{
	int i;
	for (i=0;i<MCL_NLEN-1;i++)
		b[i]=a[i];

	b[MCL_NLEN-1]=a[MCL_NLEN-1]&BMASK; /* top word normalized */
	b[MCL_NLEN]=a[MCL_NLEN-1]>>MCL_BASEBITS;

	for (i=MCL_NLEN+1;i<DMCL_NLEN;i++) b[i]=0;
#ifdef MCL_DEBUG_NORM
	b[DMCL_NLEN]=a[MCL_NLEN];
#endif
}

/* Copy MCL_BIG to top half of DMCL_BIG */
void MCL_BIG_dsucopy(DMCL_BIG b,MCL_BIG a)
{
	int i;
	for (i=0;i<MCL_NLEN;i++)
		b[i]=0;
	for (i=MCL_NLEN;i<DMCL_NLEN;i++)
		b[i]=a[i-MCL_NLEN];
#ifdef MCL_DEBUG_NORM
	b[DMCL_NLEN]=a[MCL_NLEN];
#endif
}

/* Copy bottom half of DMCL_BIG to MCL_BIG */
void MCL_BIG_sdcopy(MCL_BIG b,DMCL_BIG a)
{
	int i;
	for (i=0;i<MCL_NLEN;i++)
		b[i]=a[i];
#ifdef MCL_DEBUG_NORM
	b[MCL_NLEN]=a[DMCL_NLEN];
#endif
}

/* Copy top half of DMCL_BIG to MCL_BIG */
void MCL_BIG_sducopy(MCL_BIG b,DMCL_BIG a)
{
	int i;
	for (i=0;i<MCL_NLEN;i++)
		b[i]=a[i+MCL_NLEN];
#ifdef MCL_DEBUG_NORM
	b[MCL_NLEN]=a[DMCL_NLEN];
#endif
}

/* Set a=0 */
void MCL_BIG_zero(MCL_BIG a)
{
	int i;
	for (i=0;i<MCL_NLEN;i++)
		a[i]=0;
#ifdef MCL_DEBUG_NORM
	a[MCL_NLEN]=0;
#endif
}

void MCL_BIG_dzero(DMCL_BIG a)
{
	int i;
	for (i=0;i<DMCL_NLEN;i++)
		a[i]=0;
#ifdef MCL_DEBUG_NORM
	a[DMCL_NLEN]=0;
#endif
}

/* set a=1 */
void MCL_BIG_one(MCL_BIG a)
{
	int i;
	a[0]=1;
	for (i=1;i<MCL_NLEN;i++)
		a[i]=0;
#ifdef MCL_DEBUG_NORM
	a[MCL_NLEN]=0; 
#endif
}



/* Set c=a+b */
/* SU= 8 */
void MCL_BIG_add(MCL_BIG c,MCL_BIG a,MCL_BIG b)
{
	int i;
	for (i=0;i<MCL_NLEN;i++)
		c[i]=a[i]+b[i];
#ifdef MCL_DEBUG_NORM
	c[MCL_NLEN]=a[MCL_NLEN]+b[MCL_NLEN]+1;
	if (c[MCL_NLEN]>=NEXCESS) printf("add problem - digit overflow %d\n",c[MCL_NLEN]);
#endif
}

/* Set c=c+d */
void MCL_BIG_inc(MCL_BIG c,int d)
{
	MCL_BIG_norm(c);
	c[0]+=(mcl_chunk)d;
#ifdef MCL_DEBUG_NORM
	c[MCL_NLEN]=1;
#endif
}

/* Set c=a-b */
/* SU= 8 */
void MCL_BIG_sub(MCL_BIG c,MCL_BIG a,MCL_BIG b)
{
	int i;
	for (i=0;i<MCL_NLEN;i++)
		c[i]=a[i]-b[i];
#ifdef MCL_DEBUG_NORM
	c[MCL_NLEN]=a[MCL_NLEN]+b[MCL_NLEN]+1;
	if (c[MCL_NLEN]>=NEXCESS) printf("sub problem - digit overflow %d\n",c[MCL_NLEN]);
#endif
}

/* SU= 8 */

void MCL_BIG_dsub(DMCL_BIG c,DMCL_BIG a,DMCL_BIG b)
{
	int i;
	for (i=0;i<DMCL_NLEN;i++)
		c[i]=a[i]-b[i];
#ifdef MCL_DEBUG_NORM
	c[DMCL_NLEN]=a[DMCL_NLEN]+b[DMCL_NLEN]+1;
	if (c[DMCL_NLEN]>=NEXCESS) printf("sub problem - digit overflow %d\n",c[DMCL_NLEN]);
#endif
}


/* Set c=c-1 */
void MCL_BIG_dec(MCL_BIG c,int d)
{
	MCL_BIG_norm(c);
	c[0]-=(mcl_chunk)d;
#ifdef MCL_DEBUG_NORM
	c[MCL_NLEN]=1;
#endif
}

/* multiplication r=a*c by c<=NEXCESS */
void MCL_BIG_imul(MCL_BIG r,MCL_BIG a,int c)
{
	int i;
	for (i=0;i<MCL_NLEN;i++) r[i]=a[i]*c;
#ifdef MCL_DEBUG_NORM
	r[MCL_NLEN]=(a[MCL_NLEN]+1)*c-1;
	if (r[MCL_NLEN]>=NEXCESS) printf("int mul problem - digit overflow %d\n",r[MCL_NLEN]);
#endif
}

/* multiplication r=a*c by larger integer - c<=FEXCESS */
/* SU= 24 */
mcl_chunk MCL_BIG_pmul(MCL_BIG r,MCL_BIG a,int c)
{
	int i;
	mcl_chunk ak,carry=0;
	MCL_BIG_norm(a);
	for (i=0;i<MCL_NLEN;i++)
	{
		ak=a[i];
		r[i]=0;
		carry=MCL_muladd(ak,(mcl_chunk)c,carry,&r[i]);
	}
#ifdef MCL_DEBUG_NORM
	r[MCL_NLEN]=0;
#endif
	return carry;
}

/* r/=3 */
/* SU= 16 */
int MCL_BIG_div3(MCL_BIG r)
{
	int i;
	mcl_chunk ak,base,carry=0;
	MCL_BIG_norm(r);
	base=((mcl_chunk)1<<MCL_BASEBITS);
	for (i=MCL_NLEN-1;i>=0;i--)
	{
		ak=(carry*base+r[i]);
		r[i]=ak/3;
		carry=ak%3;
	}
	return (int)carry;
}

/* multiplication c=a*b by even larger integer b>FEXCESS, resulting in DMCL_BIG */
/* SU= 24 */
void MCL_BIG_pxmul(DMCL_BIG c,MCL_BIG a,int b)
{
	int j;
	mcl_chunk carry;
	MCL_BIG_dzero(c);
	carry=0;
	for (j=0;j<MCL_NLEN;j++)
		carry=MCL_muladd(a[j],(mcl_chunk)b,carry,&c[j]);
	c[MCL_NLEN]=carry;
#ifdef MCL_DEBUG_NORM
	c[DMCL_NLEN]=0;
#endif
}

/* Set c=a*b */
/* SU= 72 */
void MCL_BIG_mul(DMCL_BIG c,MCL_BIG a,MCL_BIG b)
{
	int i,j;
#ifdef mcl_dchunk
	mcl_dchunk t,co;
	mcl_dchunk s[2*MCL_NLEN-1];
	mcl_dchunk d[MCL_NLEN];
        int k;
#endif

	MCL_BIG_norm(a);  /* needed here to prevent overflow from addition of partial products */
	MCL_BIG_norm(b);

/* Faster to Combafy it.. Let the compiler unroll the loops! */

#ifdef MCL_COMBA

	for (i=0;i<MCL_NLEN;i++)
		d[i]=(mcl_dchunk)a[i]*b[i];

	s[0]=d[0];
	for (i=1;i<MCL_NLEN;i++)
		s[i]=s[i-1]+d[i];

	s[2*MCL_NLEN-2]=d[MCL_NLEN-1];
	for (i=2*MCL_NLEN-3;i>=MCL_NLEN;i--)
		s[i]=s[i+1]+d[i-MCL_NLEN+1];

	c[0]=s[0]&BMASK; co=s[0]>>MCL_BASEBITS;

	for (j=1;j<MCL_NLEN;j++)
	{
		t=co; t+=s[j]; 
		k=j;
		for (i=0;i<k;i++ )
		{
			t+=(mcl_dchunk)(a[i]-a[k])*(b[k]-b[i]);
			k--;
		}
		c[j]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;
	}

	for (j=MCL_NLEN;j<2*MCL_NLEN-2;j++)
	{
		t=co; t+=s[j]; 
		k=MCL_NLEN-1;
		for (i=j-MCL_NLEN+1;i<k;i++)
		{
			t+=(mcl_dchunk)(a[i]-a[k])*(b[k]-b[i]);
			k--;
		}
		c[j]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;
	}

	t=(mcl_dchunk)s[2*MCL_NLEN-2]+co;
	c[2*MCL_NLEN-2]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;
	c[2*MCL_NLEN-1]=(mcl_chunk)co;

#else
	mcl_chunk carry;
	MCL_BIG_dzero(c);
	for (i=0;i<MCL_NLEN;i++)
	{
		carry=0;
		for (j=0;j<MCL_NLEN;j++)
			carry=MCL_muladd(a[i],b[j],carry,&c[i+j]);
        c[MCL_NLEN+i]=carry;
	}
#endif

#ifdef MCL_DEBUG_NORM
	c[DMCL_NLEN]=0;
#endif
}

/* .. if you know the result will fit in a MCL_BIG, c must be distinct from a and b */
/* SU= 40 */
void MCL_BIG_smul(MCL_BIG c,MCL_BIG a,MCL_BIG b)
{
	int i,j;
	mcl_chunk carry;
	MCL_BIG_norm(a);  
	MCL_BIG_norm(b);

	MCL_BIG_zero(c);
	for (i=0;i<MCL_NLEN;i++)
	{
		carry=0;
		for (j=0;j<MCL_NLEN;j++)
			if (i+j<MCL_NLEN) carry=MCL_muladd(a[i],b[j],carry,&c[i+j]);
	}
#ifdef MCL_DEBUG_NORM
	c[MCL_NLEN]=0;
#endif

}

/* Set c=a*a */ 
/* SU= 80 */
void MCL_BIG_sqr(DMCL_BIG c,MCL_BIG a)
{
	int i,j;
#ifdef mcl_dchunk
	mcl_dchunk t,co;
#endif

	MCL_BIG_norm(a);

/* Note 2*a[i] in loop below and extra addition */

#ifdef MCL_COMBA

	t=(mcl_dchunk)a[0]*a[0];
	c[0]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;
	t=(mcl_dchunk)a[1]*a[0]; t+=t; t+=co; 
	c[1]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;

#if MCL_NLEN%2==1
	for (j=2;j<MCL_NLEN-1;j+=2)
	{
		t=(mcl_dchunk)a[j]*a[0]; for (i=1;i<(j+1)/2;i++) t+=(mcl_dchunk)a[j-i]*a[i]; t+=t; t+=co;  t+=(mcl_dchunk)a[j/2]*a[j/2];
		c[j]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;
		t=(mcl_dchunk)a[j+1]*a[0]; for (i=1;i<(j+2)/2;i++) t+=(mcl_dchunk)a[j+1-i]*a[i]; t+=t; t+=co; 
		c[j+1]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;	
	}
	j=MCL_NLEN-1;
	t=(mcl_dchunk)a[j]*a[0]; for (i=1;i<(j+1)/2;i++) t+=(mcl_dchunk)a[j-i]*a[i]; t+=t; t+=co;  t+=(mcl_dchunk)a[j/2]*a[j/2];
	c[j]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;

#else
	for (j=2;j<MCL_NLEN;j+=2)
	{
		t=(mcl_dchunk)a[j]*a[0]; for (i=1;i<(j+1)/2;i++) t+=(mcl_dchunk)a[j-i]*a[i]; t+=t; t+=co;  t+=(mcl_dchunk)a[j/2]*a[j/2];
		c[j]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;
		t=(mcl_dchunk)a[j+1]*a[0]; for (i=1;i<(j+2)/2;i++) t+=(mcl_dchunk)a[j+1-i]*a[i]; t+=t; t+=co; 
		c[j+1]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;	
	}

#endif

#if MCL_NLEN%2==1
	j=MCL_NLEN;
	t=(mcl_dchunk)a[MCL_NLEN-1]*a[j-MCL_NLEN+1]; for (i=j-MCL_NLEN+2;i<(j+1)/2;i++) t+=(mcl_dchunk)a[j-i]*a[i]; t+=t; t+=co; 
	c[j]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;
	for (j=MCL_NLEN+1;j<DMCL_NLEN-2;j+=2)
	{
		t=(mcl_dchunk)a[MCL_NLEN-1]*a[j-MCL_NLEN+1]; for (i=j-MCL_NLEN+2;i<(j+1)/2;i++) t+=(mcl_dchunk)a[j-i]*a[i]; t+=t; t+=co; t+=(mcl_dchunk)a[j/2]*a[j/2];
		c[j]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;
		t=(mcl_dchunk)a[MCL_NLEN-1]*a[j-MCL_NLEN+2]; for (i=j-MCL_NLEN+3;i<(j+2)/2;i++) t+=(mcl_dchunk)a[j+1-i]*a[i]; t+=t; t+=co;
		c[j+1]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;
	}
#else
	for (j=MCL_NLEN;j<DMCL_NLEN-2;j+=2)
	{
		t=(mcl_dchunk)a[MCL_NLEN-1]*a[j-MCL_NLEN+1]; for (i=j-MCL_NLEN+2;i<(j+1)/2;i++) t+=(mcl_dchunk)a[j-i]*a[i]; t+=t; t+=co; t+=(mcl_dchunk)a[j/2]*a[j/2];
		c[j]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;
		t=(mcl_dchunk)a[MCL_NLEN-1]*a[j-MCL_NLEN+2]; for (i=j-MCL_NLEN+3;i<(j+2)/2;i++) t+=(mcl_dchunk)a[j+1-i]*a[i]; t+=t; t+=co;
		c[j+1]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;
	}

#endif
	
	t=(mcl_dchunk)a[MCL_NLEN-1]*a[MCL_NLEN-1]+co;
	c[DMCL_NLEN-2]=(mcl_chunk)t&BMASK; co=t>>MCL_BASEBITS;
	c[DMCL_NLEN-1]=(mcl_chunk)co;

#else
	mcl_chunk carry;
	MCL_BIG_dzero(c);
	for (i=0;i<MCL_NLEN;i++)
	{
		carry=0;
		for (j=i+1;j<MCL_NLEN;j++)
			carry=MCL_muladd(a[i],a[j],carry,&c[i+j]);
        c[MCL_NLEN+i]=carry;
	}

	for (i=0;i<DMCL_NLEN;i++) c[i]*=2;

	for (i=0;i<MCL_NLEN;i++)
		c[2*i+1]+=MCL_muladd(a[i],a[i],0,&c[2*i]);

	MCL_BIG_dnorm(c); 
#endif


#ifdef MCL_DEBUG_NORM
	c[DMCL_NLEN]=0;
#endif

}

/* General shift left of a by n bits */
/* a MUST be normalised */
/* SU= 32 */
void MCL_BIG_shl(MCL_BIG a,int k)
{
	int i; 
	int n=k%MCL_BASEBITS;
	int m=k/MCL_BASEBITS;

	a[MCL_NLEN-1]=((a[MCL_NLEN-1-m]<<n))|(a[MCL_NLEN-m-2]>>(MCL_BASEBITS-n));

	for (i=MCL_NLEN-2;i>m;i--)
		a[i]=((a[i-m]<<n)&BMASK)|(a[i-m-1]>>(MCL_BASEBITS-n));
	a[m]=(a[0]<<n)&BMASK; 
	for (i=0;i<m;i++) a[i]=0;

}

/* Fast shift left of a by n bits, where n less than a word, Return excess (but store it as well) */
/* a MUST be normalised */
/* SU= 16 */
mcl_chunk MCL_BIG_fshl(MCL_BIG a,int n)
{
	int i; 

	a[MCL_NLEN-1]=((a[MCL_NLEN-1]<<n))|(a[MCL_NLEN-2]>>(MCL_BASEBITS-n)); /* top word not masked */
	for (i=MCL_NLEN-2;i>0;i--)
		a[i]=((a[i]<<n)&BMASK)|(a[i-1]>>(MCL_BASEBITS-n));
	a[0]=(a[0]<<n)&BMASK; 

	return (a[MCL_NLEN-1]>>((8*MCL_MODBYTES)%MCL_BASEBITS)); /* return excess - only used in ff.c */
}

/* double length left shift of a by k bits - k can be > MCL_BASEBITS , a MUST be normalised */
/* SU= 32 */
void MCL_BIG_dshl(DMCL_BIG a,int k)
{
	int i; 
	int n=k%MCL_BASEBITS;
	int m=k/MCL_BASEBITS;
	
	a[DMCL_NLEN-1]=((a[DMCL_NLEN-1-m]<<n))|(a[DMCL_NLEN-m-2]>>(MCL_BASEBITS-n));

	for (i=DMCL_NLEN-2;i>m;i--)
		a[i]=((a[i-m]<<n)&BMASK)|(a[i-m-1]>>(MCL_BASEBITS-n));
	a[m]=(a[0]<<n)&BMASK; 
	for (i=0;i<m;i++) a[i]=0;

}

/* General shift rightof a by k bits */
/* a MUST be normalised */ 
/* SU= 32 */
void MCL_BIG_shr(MCL_BIG a,int k)
{
	int i; 
	int n=k%MCL_BASEBITS;
	int m=k/MCL_BASEBITS;	
	for (i=0;i<MCL_NLEN-m-1;i++)
		a[i]=(a[m+i]>>n)|((a[m+i+1]<<(MCL_BASEBITS-n))&BMASK);
	a[MCL_NLEN-m-1]=a[MCL_NLEN-1]>>n;
	for (i=MCL_NLEN-m;i<MCL_NLEN;i++) a[i]=0;

}

/* Faster shift right of a by k bits. Return shifted out part */
/* a MUST be normalised */ 
/* SU= 16 */
mcl_chunk MCL_BIG_fshr(MCL_BIG a,int k)
{
	int i; 
	mcl_chunk r=a[0]&(((mcl_chunk)1<<k)-1); /* shifted out part */
	for (i=0;i<MCL_NLEN-1;i++)
		a[i]=(a[i]>>k)|((a[i+1]<<(MCL_BASEBITS-k))&BMASK);
	a[MCL_NLEN-1]=a[MCL_NLEN-1]>>k;
	return r;
}

/* double length right shift of a by k bits - can be > MCL_BASEBITS */
/* SU= 32 */
void MCL_BIG_dshr(DMCL_BIG a,int k)
{
	int i; 
	int n=k%MCL_BASEBITS;
	int m=k/MCL_BASEBITS;	
	for (i=0;i<DMCL_NLEN-m-1;i++)
		a[i]=(a[m+i]>>n)|((a[m+i+1]<<(MCL_BASEBITS-n))&BMASK);
	a[DMCL_NLEN-m-1]=a[DMCL_NLEN-1]>>n;
	for (i=DMCL_NLEN-m;i<DMCL_NLEN;i++ ) a[i]=0;
}

/* Split DMCL_BIG d into two MCL_BIGs t|b. Split happens at n bits, where n falls into MCL_NLEN word */
/* d MUST be normalised */ 
/* SU= 24 */
void MCL_BIG_split(MCL_BIG t,MCL_BIG b,DMCL_BIG d,int n)
{
	int i;
	mcl_chunk nw,carry;
	int m=n%MCL_BASEBITS;
//	MCL_BIG_dnorm(d);

	for (i=0;i<MCL_NLEN-1;i++) b[i]=d[i];

	b[MCL_NLEN-1]=d[MCL_NLEN-1]&(((mcl_chunk)1<<m)-1);

	if (t!=b)
	{
		carry=(d[DMCL_NLEN-1]<<(MCL_BASEBITS-m)); 
		for (i=DMCL_NLEN-2;i>=MCL_NLEN-1;i--)
		{
			nw=(d[i]>>m)|carry;
			carry=(d[i]<<(MCL_BASEBITS-m))&BMASK;
			t[i-MCL_NLEN+1]=nw;
		}
	}
#ifdef MCL_DEBUG_NORM
		t[MCL_BS]=0;
		b[MCL_BS]=0;
#endif

}

/* you gotta keep the sign of carry! Look - no branching! */
/* Note that sign bit is needed to disambiguate between +ve and -ve values */
/* normalise MCL_BIG - force all digits < 2^MCL_BASEBITS */
mcl_chunk MCL_BIG_norm(MCL_BIG a)
{
	int i;
	mcl_chunk d,carry=0;
	for (i=0;i<MCL_NLEN-1;i++)
	{
		d=a[i]+carry;
		a[i]=d&BMASK;
		carry=d>>MCL_BASEBITS;
	}
	a[MCL_NLEN-1]=(a[MCL_NLEN-1]+carry);

#ifdef MCL_DEBUG_NORM
	a[MCL_BS]=0;
#endif
	return (a[MCL_NLEN-1]>>((8*MCL_MODBYTES)%MCL_BASEBITS));  /* only used in ff.c */
}

void MCL_BIG_dnorm(DMCL_BIG a)
{
	int i;
	mcl_chunk d,carry=0;;
	for (i=0;i<DMCL_NLEN-1;i++)
	{
		d=a[i]+carry;
		a[i]=d&BMASK;
		carry=d>>MCL_BASEBITS;
	}
	a[DMCL_NLEN-1]=(a[DMCL_NLEN-1]+carry);
#ifdef MCL_DEBUG_NORM
	a[DMCL_NLEN]=0;
#endif
}

/* Compare a and b. Return 1 for a>b, -1 for a<b, 0 for a==b */
/* a and b MUST be normalised before call */ 
int MCL_BIG_comp(MCL_BIG a,MCL_BIG b)
{
	int i;
	for (i=MCL_NLEN-1;i>=0;i--)
	{
		if (a[i]==b[i]) continue;
		if (a[i]>b[i]) return 1;
		else  return -1;
	}
	return 0;
}

int MCL_BIG_dcomp(DMCL_BIG a,DMCL_BIG b)
{
	int i;
	for (i=DMCL_NLEN-1;i>=0;i--)
	{
		if (a[i]==b[i]) continue;
		if (a[i]>b[i]) return 1;
		else  return -1;
	}
	return 0;
}

/* return number of bits in a */
/* SU= 8 */
int MCL_BIG_nbits(MCL_BIG a)
{
	int bts,k=MCL_NLEN-1;
	mcl_chunk c;
	MCL_BIG_norm(a);
	while (k>=0 && a[k]==0) k--;
	if (k<0) return 0;
    bts=MCL_BASEBITS*k;
	c=a[k];
	while (c!=0) {c/=2; bts++;}
	return bts;
}

/* SU= 8 */
int MCL_BIG_dnbits(MCL_BIG a)
{
	int bts,k=DMCL_NLEN-1;
	mcl_chunk c;
	MCL_BIG_dnorm(a);
	while (a[k]==0 && k>=0) k--;
	if (k<0) return 0;
    bts=MCL_BASEBITS*k;
	c=a[k];
	while (c!=0) {c/=2; bts++;}
	return bts;
}


/* Set b=b mod c */
/* SU= 16 */
void MCL_BIG_mod(MCL_BIG b,MCL_BIG c)
{
	int k=0;  

	MCL_BIG_norm(b);
	if (MCL_BIG_comp(b,c)<0) 
		return;
	do
	{
		MCL_BIG_fshl(c,1);
		k++;
	} while (MCL_BIG_comp(b,c)>=0);

	while (k>0)
	{
		MCL_BIG_fshr(c,1);
		if (MCL_BIG_comp(b,c)>=0)
		{
			MCL_BIG_sub(b,b,c);
			MCL_BIG_norm(b);
		}
		k--;
	}
}

/* Set a=b mod c, b is destroyed. Slow but rarely used. */
/* SU= 96 */
void MCL_BIG_dmod(MCL_BIG a,DMCL_BIG b,MCL_BIG c)
{
	int k=0;
	mcl_chunk m[DMCL_BS];
	MCL_BIG_dnorm(b);
	MCL_BIG_dscopy(m,c);

	if (MCL_BIG_dcomp(b,m)<0)
	{
		MCL_BIG_sdcopy(a,b);
		return;
	}

	do
	{
		MCL_BIG_dshl(m,1);
		k++;
	} while (MCL_BIG_dcomp(b,m)>=0);

	while (k>0)
	{
		MCL_BIG_dshr(m,1);
		if (MCL_BIG_dcomp(b,m)>=0)
		{
			MCL_BIG_dsub(b,b,m);
			MCL_BIG_dnorm(b);
		}
		k--;
	}
	MCL_BIG_sdcopy(a,b);
}

/* Set a=b/c,  b is destroyed. Slow but rarely used. */
/* SU= 136 */
void MCL_BIG_ddiv(MCL_BIG a,DMCL_BIG b,MCL_BIG c)
{
	int k=0;
	mcl_chunk m[DMCL_BS];
	mcl_chunk e[MCL_BS];
	MCL_BIG_dnorm(b);
	MCL_BIG_dscopy(m,c);

	MCL_BIG_zero(a);
	MCL_BIG_zero(e); MCL_BIG_inc(e,1);

	while (MCL_BIG_dcomp(b,m)>=0)
	{
		MCL_BIG_fshl(e,1);
		MCL_BIG_dshl(m,1);
		k++;
	}

	while (k>0)
	{
		MCL_BIG_dshr(m,1);
		MCL_BIG_fshr(e,1);
		if (MCL_BIG_dcomp(b,m)>=0)
		{
			MCL_BIG_add(a,a,e);
			MCL_BIG_norm(a);
			MCL_BIG_dsub(b,b,m);
			MCL_BIG_dnorm(b);
		}
		k--;
	}
}

/* SU= 136 */

void MCL_BIG_sdiv(MCL_BIG a,MCL_BIG c)
{
	int k=0;
	mcl_chunk m[MCL_BS],e[MCL_BS],b[MCL_BS];
	MCL_BIG_norm(a);
	MCL_BIG_copy(b,a);
	MCL_BIG_copy(m,c);

	MCL_BIG_zero(a);
	MCL_BIG_zero(e); MCL_BIG_inc(e,1);

	while (MCL_BIG_comp(b,m)>=0)
	{
		MCL_BIG_fshl(e,1);
		MCL_BIG_fshl(m,1);
		k++;
	}

	while (k>0)
	{
		MCL_BIG_fshr(m,1);
		MCL_BIG_fshr(e,1);
		if (MCL_BIG_comp(b,m)>=0)
		{
			MCL_BIG_add(a,a,e);
			MCL_BIG_norm(a);
			MCL_BIG_sub(b,b,m);
			MCL_BIG_norm(b);
		}
		k--;
	}
}

/* return LSB of a */
int MCL_BIG_parity(MCL_BIG a)
{
	return a[0]%2;
}

/* return n-th bit of a */
/* SU= 16 */
int MCL_BIG_bit(MCL_BIG a,int n)
{
	if (a[n/MCL_BASEBITS]&((mcl_chunk)1<<(n%MCL_BASEBITS))) return 1;
	else return 0;
}

/* return NAF value as +/- 1, 3 or 5. x and x3 should be normed. 
nbs is number of bits processed, and nzs is number of trailing 0s detected */
/* SU= 32 */
int MCL_BIG_nafbits(MCL_BIG x,MCL_BIG x3,int i,int *nbs,int *nzs)
{
	int j,r,nb;

	nb=MCL_BIG_bit(x3,i)-MCL_BIG_bit(x,i);
	*nbs=1;
	*nzs=0;
	if (nb==0) return 0;
	if (i==0) return nb;

    if (nb>0) r=1;
    else      r=(-1);

    for (j=i-1;j>0;j--)
    {
        (*nbs)++;
        r*=2;
        nb=MCL_BIG_bit(x3,j)-MCL_BIG_bit(x,j);
        if (nb>0) r+=1;
        if (nb<0) r-=1;
        if (abs(r)>5) break;
    }

	if (r%2!=0 && j!=0)
    { /* backtrack */
        if (nb>0) r=(r-1)/2;
        if (nb<0) r=(r+1)/2;
        (*nbs)--;
    }
    
    while (r%2==0)
    { /* remove trailing zeros */
        r/=2;
        (*nzs)++;
        (*nbs)--;
    }     
    return r;
}

/* return last n bits of a, where n is small < MCL_BASEBITS */
/* SU= 16 */
int MCL_BIG_lastbits(MCL_BIG a,int n)
{
	int msk=(1<<n)-1;
	MCL_BIG_norm(a);
	return ((int)a[0])&msk;
}

/* get 8*MCL_MODBYTES size random number */
void MCL_BIG_random(MCL_BIG m,csprng *rng)
{
	int i,b,j=0,r=0;

	MCL_BIG_zero(m);
/* generate random MCL_BIG */ 
	for (i=0;i<8*MCL_MODBYTES;i++) 
	{
		if (j==0) r=MCL_RAND_byte(rng);
		else r>>=1;
		b=r&1;
		MCL_BIG_shl(m,1); m[0]+=b; 
		j++; j&=7; 
	}

#ifdef MCL_DEBUG_NORM
	m[MCL_NLEN]=0;
#endif
}

/* get random MCL_BIG from rng, modulo q. Done one bit at a time, so its portable */

void MCL_BIG_randomnum(MCL_BIG m,MCL_BIG q,csprng *rng)
{
	int i,b,j=0,r=0;
	mcl_chunk d[DMCL_BS];
	MCL_BIG_dzero(d);
/* generate random DMCL_BIG */ 
	for (i=0;i<16*MCL_MODBYTES;i++)
	{
		if (j==0) r=MCL_RAND_byte(rng);
		else r>>=1;
		b=r&1;
		MCL_BIG_dshl(d,1); d[0]+=b; 
		j++; j&=7; 
	}
/* reduce modulo a MCL_BIG. Removes bias */	
	MCL_BIG_dmod(m,d,q);
#ifdef MCL_DEBUG_NORM
	m[MCL_NLEN]=0;
#endif
}

/* Set r=a*b mod m */
/* SU= 96 */
void MCL_BIG_modmul(MCL_BIG r,MCL_BIG a,MCL_BIG b,MCL_BIG m)
{
	mcl_chunk d[DMCL_BS];
	MCL_BIG_mod(a,m);
	MCL_BIG_mod(b,m);
	MCL_BIG_mul(d,a,b);
	MCL_BIG_dmod(r,d,m);
}

/* Set a=a*a mod m */
/* SU= 88 */
void MCL_BIG_modsqr(MCL_BIG r,MCL_BIG a,MCL_BIG m)
{
	mcl_chunk d[DMCL_BS];
	MCL_BIG_mod(a,m);
	MCL_BIG_sqr(d,a);
	MCL_BIG_dmod(r,d,m);
}

/* Set r=-a mod m */
/* SU= 16 */
void MCL_BIG_modneg(MCL_BIG r,MCL_BIG a,MCL_BIG m)
{
	MCL_BIG_mod(a,m);
	MCL_BIG_sub(r,m,a);
}

/* Set a=a/b mod m */
/* SU= 136 */
void MCL_BIG_moddiv(MCL_BIG r,MCL_BIG a,MCL_BIG b,MCL_BIG m)
{
	mcl_chunk d[DMCL_BS];
	mcl_chunk z[MCL_BS];
	MCL_BIG_mod(a,m);
	MCL_BIG_invmodp(z,b,m);
	MCL_BIG_mul(d,a,z);
	MCL_BIG_dmod(r,d,m);
}

/* Get jacobi Symbol (a/p). Returns 0, 1 or -1 */
/* SU= 216 */
int MCL_BIG_jacobi(MCL_BIG a,MCL_BIG p)
{
	int n8,k,m=0;
	mcl_chunk t[MCL_BS],x[MCL_BS],n[MCL_BS],zilch[MCL_BS],one[MCL_BS];
	MCL_BIG_one(one);
	MCL_BIG_zero(zilch);
	if (MCL_BIG_parity(p)==0 || MCL_BIG_comp(a,zilch)==0 || MCL_BIG_comp(p,one)<=0) return 0;
	MCL_BIG_norm(a); 
	MCL_BIG_copy(x,a);
	MCL_BIG_copy(n,p);
	MCL_BIG_mod(x,p);

	while (MCL_BIG_comp(n,one)>0)
	{
		if (MCL_BIG_comp(x,zilch)==0) return 0;
		n8=MCL_BIG_lastbits(n,3);
		k=0;
		while (MCL_BIG_parity(x)==0)
		{
			k++;
			MCL_BIG_shr(x,1);
		}
		if (k%2==1) m+=(n8*n8-1)/8;
		m+=(n8-1)*(MCL_BIG_lastbits(x,2)-1)/4;
		MCL_BIG_copy(t,n);

		MCL_BIG_mod(t,x);
		MCL_BIG_copy(n,x);
		MCL_BIG_copy(x,t);
		m%=2;

	}
	if (m==0) return 1;
	else return -1;
}

/* Set r=1/a mod p. Binary method */
/* SU= 240 */
void MCL_BIG_invmodp(MCL_BIG r,MCL_BIG a,MCL_BIG p)
{
	mcl_chunk u[MCL_BS],v[MCL_BS],x1[MCL_BS],x2[MCL_BS],t[MCL_BS],one[MCL_BS];
	MCL_BIG_mod(a,p);
	MCL_BIG_copy(u,a);
	MCL_BIG_copy(v,p);
	MCL_BIG_one(one);
	MCL_BIG_copy(x1,one);
	MCL_BIG_zero(x2);

	while (MCL_BIG_comp(u,one)!=0 && MCL_BIG_comp(v,one)!=0)
	{
		while (MCL_BIG_parity(u)==0)
		{
			MCL_BIG_shr(u,1);
			if (MCL_BIG_parity(x1)!=0)
			{
				MCL_BIG_add(x1,p,x1);
				MCL_BIG_norm(x1);
			}
			MCL_BIG_shr(x1,1);
		}
		while (MCL_BIG_parity(v)==0)
		{
			MCL_BIG_shr(v,1);
			if (MCL_BIG_parity(x2)!=0)
			{
				MCL_BIG_add(x2,p,x2);
				MCL_BIG_norm(x2);
			}
			MCL_BIG_shr(x2,1);
		}
		if (MCL_BIG_comp(u,v)>=0)
		{
			MCL_BIG_sub(u,u,v);
			MCL_BIG_norm(u);
			if (MCL_BIG_comp(x1,x2)>=0) MCL_BIG_sub(x1,x1,x2);
			else
			{
				MCL_BIG_sub(t,p,x2);
				MCL_BIG_add(x1,x1,t);
			}
			MCL_BIG_norm(x1);
		}
		else
		{
			MCL_BIG_sub(v,v,u);
			MCL_BIG_norm(v);
			if (MCL_BIG_comp(x2,x1)>=0) MCL_BIG_sub(x2,x2,x1);
			else
			{
				MCL_BIG_sub(t,p,x1);
				MCL_BIG_add(x2,x2,t);
			}
			MCL_BIG_norm(x2);
		}
	}
	if (MCL_BIG_comp(u,one)==0)
		MCL_BIG_copy(r,x1);
	else
		MCL_BIG_copy(r,x2);
}
