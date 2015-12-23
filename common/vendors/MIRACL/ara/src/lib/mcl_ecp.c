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


/* ARAcrypt Elliptic Curve Functions */

//#define HAS_MAIN

#include "mcl_arch.h"
#include "mcl_config.h"
#include "mcl_big.h"
#include "mcl_fp.h"
#include "mcl_ecp.h"

#define MCL_MODBYTES (1+(MCL_MBITS-1)/8) /**< Number of bytes in MCL_Modulus */
#define MCL_NLEN (1+((MCL_MBITS-1)/MCL_BASEBITS))	/**< Number of words in MCL_BIG. */

/* test for P=O point-at-infinity */
int MCL_ECP_isinf(MCL_ECP *P)
{
#if MCL_CURVETYPE==MCL_EDWARDS
	MCL_FP_reduce(P->x);
	MCL_FP_reduce(P->y);
	MCL_FP_reduce(P->z);
	return (MCL_BIG_iszilch(P->x) && MCL_BIG_comp(P->y,P->z)==0);
#else
	return P->inf;
#endif
}

/* Conditional swap of P and Q dependant on d */
static void ECP_cswap(MCL_ECP *P,MCL_ECP *Q,int d)
{
	MCL_BIG_cswap(P->x,Q->x,d);
#if MCL_CURVETYPE!=MCL_MONTGOMERY
	MCL_BIG_cswap(P->y,Q->y,d);
#endif
	MCL_BIG_cswap(P->z,Q->z,d);
#if MCL_CURVETYPE!=MCL_EDWARDS
	d=~(d-1);
	d=d&(P->inf^Q->inf);
	P->inf^=d;
	Q->inf^=d;
#endif
}

/* Conditional move Q to P dependant on d */
static void ECP_cmove(MCL_ECP *P,MCL_ECP *Q,int d)
{
	MCL_BIG_cmove(P->x,Q->x,d);
#if MCL_CURVETYPE!=MCL_MONTGOMERY
	MCL_BIG_cmove(P->y,Q->y,d);
#endif
	MCL_BIG_cmove(P->z,Q->z,d);
#if MCL_CURVETYPE!=MCL_EDWARDS
	d=~(d-1);
	P->inf^=(P->inf^Q->inf)&d;
#endif
}

/* return 1 if b==c, no branching */
static int teq(sign32 b,sign32 c)
{
	sign32 x=b^c;
	x-=1;  // if x=0, x now -1
	return (int)((x>>31)&1);
}

/* Constant time select from pre-computed table */
static void ECP_select(MCL_ECP *P,MCL_ECP W[],sign32 b)
{
  MCL_ECP MP; 
  sign32 m=b>>31;
  sign32 babs=(b^m)-m;

  babs=(babs-1)/2;

  ECP_cmove(P,&W[0],teq(babs,0));  // conditional move
  ECP_cmove(P,&W[1],teq(babs,1));
  ECP_cmove(P,&W[2],teq(babs,2));
  ECP_cmove(P,&W[3],teq(babs,3));
  ECP_cmove(P,&W[4],teq(babs,4));
  ECP_cmove(P,&W[5],teq(babs,5));
  ECP_cmove(P,&W[6],teq(babs,6));
  ECP_cmove(P,&W[7],teq(babs,7));
 
  MCL_ECP_copy(&MP,P);
  MCL_ECP_neg(&MP);  // minus P
  ECP_cmove(P,&MP,(int)(m&1));
}

/* Test P == Q */
/* SU=168 */
int MCL_ECP_equals(MCL_ECP *P,MCL_ECP *Q) 
{
#if MCL_CURVETYPE==MCL_WEIERSTRASS
	mcl_chunk pz2[MCL_BS],qz2[MCL_BS],a[MCL_BS],b[MCL_BS];
	if (MCL_ECP_isinf(P) && MCL_ECP_isinf(Q)) return 1;
	if (MCL_ECP_isinf(P) || MCL_ECP_isinf(Q)) return 0;

	MCL_FP_sqr(pz2,P->z); MCL_FP_sqr(qz2,Q->z); 

	MCL_FP_mul(a,P->x,qz2); 
	MCL_FP_mul(b,Q->x,pz2); 
	MCL_FP_reduce(a);        
	MCL_FP_reduce(b);		
	if (MCL_BIG_comp(a,b)!=0) return 0;

	MCL_FP_mul(a,P->y,qz2); 
	MCL_FP_mul(a,a,Q->z);   
	MCL_FP_mul(b,Q->y,pz2); 
	MCL_FP_mul(b,b,P->z);   
	MCL_FP_reduce(a);        
	MCL_FP_reduce(b);       
	if (MCL_BIG_comp(a,b)!=0) return 0;
	return 1;
#else
	mcl_chunk a[MCL_BS],b[MCL_BS];
	if (MCL_ECP_isinf(P) && MCL_ECP_isinf(Q)) return 1;
	if (MCL_ECP_isinf(P) || MCL_ECP_isinf(Q)) return 0;

	MCL_FP_mul(a,P->x,Q->z);
	MCL_FP_mul(b,Q->x,P->z);
	MCL_FP_reduce(a);
	MCL_FP_reduce(b);
	if (MCL_BIG_comp(a,b)!=0) return 0;

#if MCL_CURVETYPE==MCL_EDWARDS
	MCL_FP_mul(a,P->y,Q->z);
	MCL_FP_mul(b,Q->y,P->z);
	MCL_FP_reduce(a);
	MCL_FP_reduce(b);
	if (MCL_BIG_comp(a,b)!=0) return 0;
#endif

	return 1;
#endif
}

/* Set P=Q */
/* SU=16 */
void MCL_ECP_copy(MCL_ECP *P,MCL_ECP *Q)
{
#if MCL_CURVETYPE!=MCL_EDWARDS
	P->inf=Q->inf;
#endif
	MCL_BIG_copy(P->x,Q->x);
#if MCL_CURVETYPE!=MCL_MONTGOMERY
	MCL_BIG_copy(P->y,Q->y);
#endif
	MCL_BIG_copy(P->z,Q->z);
}

/* Set P=-Q */
#if MCL_CURVETYPE!=MCL_MONTGOMERY
/* SU=8 */
void MCL_ECP_neg(MCL_ECP *P)
{
	if (MCL_ECP_isinf(P)) return;
#if MCL_CURVETYPE==MCL_WEIERSTRASS
	MCL_FP_neg(P->y,P->y); 
	MCL_BIG_norm(P->y);
#else
	MCL_FP_neg(P->x,P->x);
	MCL_BIG_norm(P->x);
#endif

}
#endif

/* Set P=O */
void MCL_ECP_inf(MCL_ECP *P)
{
#if MCL_CURVETYPE==MCL_EDWARDS
	MCL_BIG_zero(P->x); MCL_FP_one(P->y); MCL_FP_one(P->z);
#else
	P->inf=1;
#endif
}

/* Calculate right Hand Side of curve equation y^2=RHS */
/* SU=56 */
void MCL_ECP_rhs(MCL_BIG v,MCL_BIG x)
{
#if MCL_CURVETYPE==MCL_WEIERSTRASS
/* x^3+Ax+B */
	mcl_chunk t[MCL_BS];
	MCL_FP_sqr(t,x); 
	MCL_FP_mul(t,t,x); 

	if (MCL_CURVE_A==-3)
	{
		MCL_FP_neg(v,x); 
		MCL_BIG_norm(v);
		MCL_BIG_imul(v,v,-MCL_CURVE_A); 
		MCL_BIG_norm(v);
		MCL_FP_add(v,t,v);         
	}
	else MCL_BIG_copy(v,t);

	MCL_BIG_rcopy(t,MCL_CURVE_B);  
	MCL_FP_nres(t);
	MCL_FP_add(v,t,v);  
	MCL_FP_reduce(v);	
#endif	

#if MCL_CURVETYPE==MCL_EDWARDS
/* (Ax^2-1)/(Bx^2-1) */
	mcl_chunk t[MCL_BS],m[MCL_BS],one[MCL_BS];
	MCL_BIG_rcopy(m,MCL_Modulus);
	MCL_FP_sqr(v,x);
	MCL_FP_one(one);
	MCL_BIG_rcopy(t,MCL_CURVE_B); MCL_FP_nres(t);
	MCL_FP_mul(t,v,t); MCL_FP_sub(t,t,one);
	if (MCL_CURVE_A==1) MCL_FP_sub(v,v,one);

	if (MCL_CURVE_A==-1)
	{
		MCL_FP_add(v,v,one);
		MCL_FP_neg(v,v);
	}
	MCL_FP_redc(v); MCL_FP_redc(t);
	MCL_BIG_moddiv(v,v,t,m);
	MCL_FP_nres(v);
#endif

#if MCL_CURVETYPE==MCL_MONTGOMERY
/* x^3+Ax^2+x */
	mcl_chunk x2[MCL_BS],x3[MCL_BS];
	MCL_FP_sqr(x2,x);
	MCL_FP_mul(x3,x2,x);
	MCL_BIG_copy(v,x);
	MCL_FP_imul(x2,x2,MCL_CURVE_A);
	MCL_FP_add(v,v,x2);
	MCL_FP_add(v,v,x3);
	MCL_FP_reduce(v);
#endif
}

/* Set P=(x,y) */

#if MCL_CURVETYPE==MCL_MONTGOMERY

/* Set P=(x,{y}) */

int MCL_ECP_set(MCL_ECP *P,MCL_BIG x)
{
	mcl_chunk m[MCL_BS],rhs[MCL_BS];
	MCL_BIG_rcopy(m,MCL_Modulus);
	MCL_BIG_copy(rhs,x);
	MCL_FP_nres(rhs);
	MCL_ECP_rhs(rhs,rhs);
	MCL_FP_redc(rhs);

	if (MCL_BIG_jacobi(rhs,m)!=1)
	{
		MCL_ECP_inf(P);
		return 0;
	}
	P->inf=0;
	MCL_BIG_copy(P->x,x); MCL_FP_nres(P->x);
	MCL_FP_one(P->z);
	return 1;
}

/* Extract x coordinate as MCL_BIG */
int MCL_ECP_get(MCL_BIG x,MCL_ECP *P)
{
	if (MCL_ECP_isinf(P)) return -1;	
	MCL_ECP_affine(P);
	MCL_BIG_copy(x,P->x);
	MCL_FP_redc(x);
	return 0;
}


#else

/* Extract (x,y) and return sign of y. If x and y are the same return only x */
/* SU=16 */
int MCL_ECP_get(MCL_BIG x,MCL_BIG y,MCL_ECP *P)
{
	int s;
#if MCL_CURVETYPE!=MCL_EDWARDS
	if (MCL_ECP_isinf(P)) return -1;	
#endif
	MCL_ECP_affine(P);

	MCL_BIG_copy(y,P->y);
	MCL_FP_redc(y);

	s=MCL_BIG_parity(y);

	MCL_BIG_copy(x,P->x);
	MCL_FP_redc(x);
	
	return s;
}

/* Set P=(x,{y}) */
/* SU=96 */
int MCL_ECP_set(MCL_ECP *P,MCL_BIG x,MCL_BIG y)
{
	mcl_chunk rhs[MCL_BS],y2[MCL_BS];
	MCL_BIG_copy(y2,y);
	MCL_FP_nres(y2);

	MCL_FP_sqr(y2,y2);
	MCL_FP_reduce(y2);

	MCL_BIG_copy(rhs,x);
	MCL_FP_nres(rhs);

	MCL_ECP_rhs(rhs,rhs);

	if (MCL_BIG_comp(y2,rhs)!=0)
	{	
		MCL_ECP_inf(P);
		return 0;
	}
#if MCL_CURVETYPE==MCL_WEIERSTRASS
	P->inf=0;
#endif
	MCL_BIG_copy(P->x,x); MCL_FP_nres(P->x);
	MCL_BIG_copy(P->y,y); MCL_FP_nres(P->y);
	MCL_FP_one(P->z);
	return 1;
}

/* Set P=(x,y), where y is calculated from x with sign s */
/* SU=136 */
int MCL_ECP_setx(MCL_ECP *P,MCL_BIG x,int s)
{
	mcl_chunk t[MCL_BS],rhs[MCL_BS],m[MCL_BS];
	MCL_BIG_rcopy(m,MCL_Modulus);

	MCL_BIG_copy(rhs,x);
	MCL_FP_nres(rhs);
	MCL_ECP_rhs(rhs,rhs);
	MCL_BIG_copy(t,rhs);
	MCL_FP_redc(t);
	if (MCL_BIG_jacobi(t,m)!=1)
	{
		MCL_ECP_inf(P);
		return 0;
	}
#if MCL_CURVETYPE==MCL_WEIERSTRASS
	P->inf=0;
#endif
	MCL_BIG_copy(P->x,x); MCL_FP_nres(P->x);

	MCL_FP_sqrt(P->y,rhs);
	MCL_BIG_copy(rhs,P->y);
	MCL_FP_redc(rhs);
	if (MCL_BIG_parity(rhs)!=s)
		MCL_FP_neg(P->y,P->y);
	MCL_FP_reduce(P->y);
	MCL_FP_one(P->z);
	return 1;
}

#endif

/* Convert P to Affine, from (x,y,z) to (x,y) */
/* SU=160 */
void MCL_ECP_affine(MCL_ECP *P) 
{
	mcl_chunk one[MCL_BS],iz[MCL_BS],m[MCL_BS];
#if MCL_CURVETYPE==MCL_WEIERSTRASS
	mcl_chunk izn[MCL_BS];
	if (MCL_ECP_isinf(P)) return;
	MCL_FP_one(one);
	if (MCL_BIG_comp(P->z,one)==0) return;
	MCL_BIG_rcopy(m,MCL_Modulus);
	MCL_FP_redc(P->z);

	MCL_BIG_invmodp(iz,P->z,m);
	MCL_FP_nres(iz);

	MCL_FP_sqr(izn,iz);
	MCL_FP_mul(P->x,P->x,izn);
	MCL_FP_mul(izn,izn,iz);
	MCL_FP_mul(P->y,P->y,izn);
	MCL_FP_reduce(P->y);

#endif
#if MCL_CURVETYPE==MCL_EDWARDS
	MCL_FP_one(one);
	if (MCL_BIG_comp(P->z,one)==0) return;
	MCL_BIG_rcopy(m,MCL_Modulus);
	MCL_FP_redc(P->z);

	MCL_BIG_invmodp(iz,P->z,m);
	MCL_FP_nres(iz);

	MCL_FP_mul(P->x,P->x,iz);
	MCL_FP_mul(P->y,P->y,iz);
	MCL_FP_reduce(P->y);

#endif
#if MCL_CURVETYPE==MCL_MONTGOMERY
	if (MCL_ECP_isinf(P)) return;
	MCL_FP_one(one);
	if (MCL_BIG_comp(P->z,one)==0) return;

	MCL_BIG_rcopy(m,MCL_Modulus);
	MCL_FP_redc(P->z);
	MCL_BIG_invmodp(iz,P->z,m);
	MCL_FP_nres(iz);

	MCL_FP_mul(P->x,P->x,iz);

#endif
	MCL_FP_reduce(P->x);
	MCL_BIG_copy(P->z,one);
}

#ifdef MCL_BUILD_TEST
/* SU=120 */
void MCL_ECP_outputxyz(MCL_ECP *P)
{
	mcl_chunk x[MCL_BS],y[MCL_BS],z[MCL_BS];
	if (MCL_ECP_isinf(P))
	{
		printf("Infinity\n");
		return;
	}
	MCL_BIG_copy(x,P->x); MCL_FP_reduce(x); MCL_FP_redc(x);
	MCL_BIG_copy(z,P->z); MCL_FP_reduce(z); MCL_FP_redc(z);

#if MCL_CURVETYPE!=MCL_MONTGOMERY
	MCL_BIG_copy(y,P->y); MCL_FP_reduce(y); MCL_FP_redc(y);	
	printf("(");MCL_BIG_output(x);printf(",");MCL_BIG_output(y);printf(",");MCL_BIG_output(z);printf(")\n");

#else
	printf("(");MCL_BIG_output(x);printf(",");MCL_BIG_output(z);printf(")\n");
#endif
}


/* SU=16 */
/* Output point P */
void MCL_ECP_output(MCL_ECP *P)
{
	if (MCL_ECP_isinf(P))
	{
		printf("Infinity\n");
		return;
	}
	MCL_ECP_affine(P);
#if MCL_CURVETYPE!=MCL_MONTGOMERY
	MCL_FP_redc(P->x); MCL_FP_redc(P->y); 
	printf("(");MCL_BIG_output(P->x);printf(",");MCL_BIG_output(P->y);printf(")\n");
	MCL_FP_nres(P->x); MCL_FP_nres(P->y); 
#else
	MCL_FP_redc(P->x); 
	printf("(");MCL_BIG_output(P->x);printf(")\n");
	MCL_FP_nres(P->x);
#endif
}

#endif //  MCL_BUILD_TEST

/* SU=88 */
/* Convert P to mcl_octet string */
void MCL_ECP_toOctet(mcl_octet *W,MCL_ECP *P)
{
#if MCL_CURVETYPE==MCL_MONTGOMERY
	mcl_chunk x[MCL_BS];
	MCL_ECP_get(x,P);
	W->len=MCL_MODBYTES+1; W->val[0]=6;
	MCL_BIG_toBytes(&(W->val[1]),x);
#else
	mcl_chunk x[MCL_BS],y[MCL_BS];
	MCL_ECP_get(x,y,P);
	W->len=2*MCL_MODBYTES+1; W->val[0]=4;
	MCL_BIG_toBytes(&(W->val[1]),x);
	MCL_BIG_toBytes(&(W->val[MCL_MODBYTES+1]),y);
#endif
}

/* SU=88 */
/* Restore P from mcl_octet string */
int MCL_ECP_fromOctet(MCL_ECP *P,mcl_octet *W)
{
#if MCL_CURVETYPE==MCL_MONTGOMERY
	mcl_chunk x[MCL_BS];
	MCL_BIG_fromBytes(x,&(W->val[1]));
    if (MCL_ECP_set(P,x)) return 1;
	return 0;
#else
	mcl_chunk x[MCL_BS],y[MCL_BS];
	MCL_BIG_fromBytes(x,&(W->val[1]));
	MCL_BIG_fromBytes(y,&(W->val[MCL_MODBYTES+1]));
    if (MCL_ECP_set(P,x,y)) return 1;
	return 0;
#endif
}


/* Set P=2P */
/* SU=272 */
void MCL_ECP_dbl(MCL_ECP *P)
{
#if MCL_CURVETYPE==MCL_WEIERSTRASS
	mcl_chunk one[MCL_BS];
	mcl_chunk w1[MCL_BS],w7[MCL_BS],w8[MCL_BS],w2[MCL_BS],w3[MCL_BS],w6[MCL_BS];
	if (MCL_ECP_isinf(P)) return;

	if (MCL_BIG_iszilch(P->y))
	{
		P->inf=1;
		return;
	}
	MCL_FP_one(one);
	MCL_BIG_zero(w6);

	if (MCL_CURVE_A==-3)
	{
		if (MCL_BIG_comp(P->z,one)==0) MCL_BIG_copy(w6,one);
		else MCL_FP_sqr(w6,P->z);  
		MCL_FP_neg(w1,w6);      
		MCL_FP_add(w3,P->x,w1); 
		MCL_FP_add(w8,P->x,w6); 
		MCL_FP_mul(w3,w3,w8);   
		MCL_BIG_imul(w8,w3,3);  
	}
	else
	{
/* assuming A=0 */
		MCL_FP_sqr(w1,P->x); 
		MCL_BIG_imul(w8,w1,3);
	}

	MCL_FP_sqr(w2,P->y);     
	MCL_FP_mul(w3,P->x,w2);  

	MCL_BIG_imul(w3,w3,4);   
	MCL_FP_neg(w1,w3);      
//#if MCL_CHUNK<64
	MCL_BIG_norm(w1);
//#endif
	MCL_FP_sqr(P->x,w8);     
	MCL_FP_add(P->x,P->x,w1); 
	MCL_FP_add(P->x,P->x,w1); 

	MCL_BIG_norm(P->x);  

	if (MCL_BIG_comp(P->z,one)==0) MCL_BIG_copy(P->z,P->y);
	else MCL_FP_mul(P->z,P->z,P->y);
	MCL_FP_add(P->z,P->z,P->z);      


	MCL_FP_add(w7,w2,w2);    
	MCL_FP_sqr(w2,w7);       

	MCL_FP_add(w2,w2,w2);    
	MCL_FP_sub(w3,w3,P->x);  
	MCL_FP_mul(P->y,w8,w3);  

	MCL_FP_sub(P->y,P->y,w2); 

	MCL_BIG_norm(P->y);
	MCL_BIG_norm(P->z);

#endif

#if MCL_CURVETYPE==MCL_EDWARDS
/* Not using square for multiplication swap, as (1) it needs more adds, and (2) it triggers more reductions */
	mcl_chunk B[MCL_BS],C[MCL_BS],D[MCL_BS],E[MCL_BS],F[MCL_BS],H[MCL_BS],J[MCL_BS];

	MCL_FP_mul(B,P->x,P->y); MCL_FP_add(B,B,B);
	MCL_FP_sqr(C,P->x);
	MCL_FP_sqr(D,P->y);
	if (MCL_CURVE_A==1) MCL_BIG_copy(E,C);
	if (MCL_CURVE_A==-1) MCL_FP_neg(E,C);
	MCL_FP_add(F,E,D);
//#if MCL_CHUNK<64
	MCL_BIG_norm(F);
//#endif
	MCL_FP_sqr(H,P->z);
	MCL_FP_add(H,H,H); MCL_FP_sub(J,F,H);
	MCL_FP_mul(P->x,B,J);
	MCL_FP_sub(E,E,D);
	MCL_FP_mul(P->y,F,E);
	MCL_FP_mul(P->z,F,J);

	MCL_BIG_norm(P->x);
	MCL_BIG_norm(P->y);
	MCL_BIG_norm(P->z);

#endif

#if MCL_CURVETYPE==MCL_MONTGOMERY
	mcl_chunk t[MCL_BS],A[MCL_BS],B[MCL_BS],AA[MCL_BS],BB[MCL_BS],C[MCL_BS];
	if (MCL_ECP_isinf(P)) return;

	MCL_FP_add(A,P->x,P->z);
	MCL_FP_sqr(AA,A);
	MCL_FP_sub(B,P->x,P->z);
	MCL_FP_sqr(BB,B);
	MCL_FP_sub(C,AA,BB);

	MCL_FP_mul(P->x,AA,BB);
	MCL_FP_imul(A,C,(MCL_CURVE_A+2)/4);
	MCL_FP_add(BB,BB,A);
	MCL_FP_mul(P->z,BB,C);

	MCL_BIG_norm(P->x);
	MCL_BIG_norm(P->z);
#endif
}

#if MCL_CURVETYPE==MCL_MONTGOMERY

/* Set P+=Q. W is difference between P and Q and is affine */
void MCL_ECP_add(MCL_ECP *P,MCL_ECP *Q,MCL_ECP *W)
{
	mcl_chunk A[MCL_BS],B[MCL_BS],C[MCL_BS],D[MCL_BS],DA[MCL_BS],CB[MCL_BS];

	MCL_FP_add(A,P->x,P->z);
	MCL_FP_sub(B,P->x,P->z);

	MCL_FP_add(C,Q->x,Q->z);
	MCL_FP_sub(D,Q->x,Q->z);

	MCL_FP_mul(DA,D,A);
	MCL_FP_mul(CB,C,B);

	MCL_FP_add(A,DA,CB); MCL_FP_sqr(A,A);
	MCL_FP_sub(B,DA,CB); MCL_FP_sqr(B,B);

	MCL_BIG_copy(P->x,A);
	MCL_FP_mul(P->z,W->x,B);

	MCL_FP_reduce(P->z);
	if (MCL_BIG_iszilch(P->z)) P->inf=1;
	else P->inf=0;

	MCL_BIG_norm(P->x);
}


#else

/* Set P+=Q */
/* SU=248 */
void MCL_ECP_add(MCL_ECP *P,MCL_ECP *Q)
{
#if MCL_CURVETYPE==MCL_WEIERSTRASS
	int aff;
	mcl_chunk one[MCL_BS],B[MCL_BS],D[MCL_BS],E[MCL_BS],C[MCL_BS],A[MCL_BS];		
	if (MCL_ECP_isinf(Q)) return;
	if (MCL_ECP_isinf(P))
	{
		MCL_ECP_copy(P,Q);
		return;
	}

	MCL_FP_one(one);
	aff=1;
	if (MCL_BIG_comp(Q->z,one)!=0) aff=0; 

	if (!aff)
	{
		MCL_FP_sqr(A,Q->z);  
		MCL_FP_mul(C,A,Q->z); 

		MCL_FP_sqr(B,P->z);  
		MCL_FP_mul(D,B,P->z); 

		MCL_FP_mul(A,P->x,A); 
		MCL_FP_mul(C,P->y,C); 
	}
	else
	{
		MCL_BIG_copy(A,P->x); 
		MCL_BIG_copy(C,P->y); 

		MCL_FP_sqr(B,P->z);
		MCL_FP_mul(D,B,P->z); 
	}

	MCL_FP_mul(B,Q->x,B); MCL_FP_sub(B,B,A); /* B=Qx.z^2-x.Qz^2 */
	MCL_FP_mul(D,Q->y,D); MCL_FP_sub(D,D,C); /* D=Qy.z^3-y.Qz^3 */

	MCL_FP_reduce(B);     
	if (MCL_BIG_iszilch(B))    
	{
		MCL_FP_reduce(D);    
		if (MCL_BIG_iszilch(D))
		{
			MCL_ECP_dbl(P);
			return;
		}
		else
		{
			MCL_ECP_inf(P);
			return;
		}
	}
	if (!aff) MCL_FP_mul(P->z,P->z,Q->z);
	MCL_FP_mul(P->z,P->z,B);	

	MCL_FP_sqr(E,B); 
	MCL_FP_mul(B,B,E);
	MCL_FP_mul(A,A,E);

	MCL_FP_add(E,A,A);
	MCL_FP_add(E,E,B);

	MCL_FP_sqr(P->x,D);
	MCL_FP_sub(P->x,P->x,E);

	MCL_FP_sub(A,A,P->x);
	MCL_FP_mul(P->y,A,D);
	MCL_FP_mul(C,C,B);
	MCL_FP_sub(P->y,P->y,C);

	MCL_BIG_norm(P->x);
	MCL_BIG_norm(P->y);
	MCL_BIG_norm(P->z);

#else
	mcl_chunk b[MCL_BS],A[MCL_BS],B[MCL_BS],C[MCL_BS],D[MCL_BS],E[MCL_BS],F[MCL_BS],G[MCL_BS];		

	MCL_BIG_rcopy(b,MCL_CURVE_B); MCL_FP_nres(b);
	MCL_FP_mul(A,P->z,Q->z);  // A=Z1*Z2

	MCL_FP_sqr(B,A);          // B=A^2
	MCL_FP_mul(C,P->x,Q->x);  // C=X1*X2
	MCL_FP_mul(D,P->y,Q->y);  // D=Y1*Y2
	MCL_FP_mul(E,C,D); MCL_FP_mul(E,E,b); // E=d*C*D

	MCL_FP_sub(F,B,E);  // F=B-E
	MCL_FP_add(G,B,E);  // G=B+E

	if (MCL_CURVE_A==1) MCL_FP_sub(E,D,C); // E=D-C

	MCL_FP_add(C,C,D);  // C=C+D *** was in wrong place

	MCL_FP_add(B,P->x,P->y); // B=X1+Y1

	MCL_FP_add(D,Q->x,Q->y); // D=X2+Y2

	MCL_FP_mul(B,B,D);       // B=(X1+Y1)(X2+Y2)
	MCL_FP_sub(B,B,C);       // B=(X1+Y1)(X2+Y2)-C-D

	MCL_FP_mul(B,B,F);       // B=F*((X1+Y1)(X2+Y2)-C-D)
	MCL_FP_mul(P->x,A,B);    // X1=A*F*((X1+Y1)(X2+Y2)-C-D)


	if (MCL_CURVE_A==1) MCL_FP_mul(C,E,G);  // C=(D-C)*G
	if (MCL_CURVE_A==-1) MCL_FP_mul(C,C,G);

	MCL_FP_mul(P->y,A,C);    // Y1=A*G*(D-C)
	MCL_FP_mul(P->z,F,G);    // Z1=F*G

	MCL_BIG_norm(P->x);
	MCL_BIG_norm(P->y);
	MCL_BIG_norm(P->z);

#endif
}

/* Set P-=Q */
/* SU=16 */
void  MCL_ECP_sub(MCL_ECP *P,MCL_ECP *Q)
{
	MCL_ECP_neg(Q);
	MCL_ECP_add(P,Q);
	MCL_ECP_neg(Q);
}

#endif


#if MCL_CURVETYPE==MCL_WEIERSTRASS
/* normalises array of points. Assumes P[0] is normalised already */

static void ECP_multiaffine(int m,MCL_ECP P[],mcl_chunk work[][MCL_BS])
{
	int i;
	mcl_chunk t1[MCL_BS],t2[MCL_BS];

	MCL_FP_one(work[0]);
	MCL_BIG_copy(work[1],P[0].z);
	for (i=2;i<m;i++)
		MCL_FP_mul(work[i],work[i-1],P[i-1].z);

	MCL_FP_mul(t1,work[m-1],P[m-1].z);
	MCL_FP_inv(t1,t1);

	MCL_BIG_copy(t2,P[m-1].z);
	MCL_FP_mul(work[m-1],work[m-1],t1);

	for (i=m-2;;i--)
    {
		if (i==0)
		{
			MCL_FP_mul(work[0],t1,t2);
			break;
		}
		MCL_FP_mul(work[i],work[i],t2);
		MCL_FP_mul(work[i],work[i],t1);
		MCL_FP_mul(t2,P[i].z,t2);
    }
/* now work[] contains inverses of all Z coordinates */

	for (i=0;i<m;i++)
	{
		MCL_FP_one(P[i].z);
		MCL_FP_sqr(t1,work[i]);
		MCL_FP_mul(P[i].x,P[i].x,t1);
		MCL_FP_mul(t1,work[i],t1);
		MCL_FP_mul(P[i].y,P[i].y,t1);
    }    
}

#endif

#if MCL_CURVETYPE!=MCL_MONTGOMERY
/* constant time multiply by small integer of length bts - use ladder */
void MCL_ECP_pinmul(MCL_ECP *P,int e,int bts)
{
	int i,b;
	MCL_ECP R0,R1;

	MCL_ECP_affine(P);
	MCL_ECP_inf(&R0);
	MCL_ECP_copy(&R1,P);

    for (i=bts-1;i>=0;i--)
	{
		b=(e>>i)&1;
		MCL_ECP_copy(P,&R1);
		MCL_ECP_add(P,&R0);
		ECP_cswap(&R0,&R1,b);
		MCL_ECP_copy(&R1,P);
		MCL_ECP_dbl(&R0);
		ECP_cswap(&R0,&R1,b);
	}
	MCL_ECP_copy(P,&R0);
	MCL_ECP_affine(P);
}
#endif

/* Set P=r*P */ 
/* SU=424 */
void MCL_ECP_mul(MCL_ECP *P,MCL_BIG e)
{
#if MCL_CURVETYPE==MCL_MONTGOMERY
/* Montgomery ladder */
	int nb,i,b;
	MCL_ECP R0,R1,D;
	if (MCL_ECP_isinf(P)) return;
	if (MCL_BIG_iszilch(e))
	{
		MCL_ECP_inf(P);
		return;
	}
	MCL_ECP_affine(P);

	MCL_ECP_copy(&R0,P);
	MCL_ECP_copy(&R1,P);
	MCL_ECP_dbl(&R1);
	MCL_ECP_copy(&D,P);

	nb=MCL_BIG_nbits(e);
    for (i=nb-2;i>=0;i--)
    {
		b=MCL_BIG_bit(e,i);
		MCL_ECP_copy(P,&R1);
		MCL_ECP_add(P,&R0,&D);
		ECP_cswap(&R0,&R1,b);
		MCL_ECP_copy(&R1,P);
		MCL_ECP_dbl(&R0);
		ECP_cswap(&R0,&R1,b);
	}
	MCL_ECP_copy(P,&R0);

#else
/* fixed size windows */
	int i,nb,s,ns;
	mcl_chunk mt[MCL_BS],t[MCL_BS];
	MCL_ECP Q,W[8],C;
	sign8 w[1+(MCL_NLEN*MCL_BASEBITS+3)/4];
#if MCL_CURVETYPE==MCL_WEIERSTRASS
	mcl_chunk work[8][MCL_BS];
#endif
	if (MCL_ECP_isinf(P)) return;	
	if (MCL_BIG_iszilch(e))
	{
		MCL_ECP_inf(P);
		return;
	}

	MCL_ECP_affine(P);

/* precompute table */

	MCL_ECP_copy(&Q,P);
	MCL_ECP_dbl(&Q);
	MCL_ECP_copy(&W[0],P); 

	for (i=1;i<8;i++)
	{
		MCL_ECP_copy(&W[i],&W[i-1]);
		MCL_ECP_add(&W[i],&Q);
	}

/* convert the table to affine */
#if MCL_CURVETYPE==MCL_WEIERSTRASS
	ECP_multiaffine(8,W,work);
#endif

/* make exponent odd - add 2P if even, P if odd */
	MCL_BIG_copy(t,e);
	s=MCL_BIG_parity(t);
	MCL_BIG_inc(t,1); MCL_BIG_norm(t); ns=MCL_BIG_parity(t); MCL_BIG_copy(mt,t); MCL_BIG_inc(mt,1); MCL_BIG_norm(mt);
	MCL_BIG_cmove(t,mt,s);
	ECP_cmove(&Q,P,ns);
	MCL_ECP_copy(&C,&Q);

	nb=1+(MCL_BIG_nbits(t)+3)/4;

/* convert exponent to signed 4-bit window */
	for (i=0;i<nb;i++)
	{
		w[i]=MCL_BIG_lastbits(t,5)-16;
		MCL_BIG_dec(t,w[i]); MCL_BIG_norm(t); 
		MCL_BIG_fshr(t,4);
	}
	w[nb]=MCL_BIG_lastbits(t,5);

	MCL_ECP_copy(P,&W[(w[nb]-1)/2]);  
	for (i=nb-1;i>=0;i--)
	{
		ECP_select(&Q,W,w[i]);
		MCL_ECP_dbl(P);
		MCL_ECP_dbl(P);
		MCL_ECP_dbl(P);
		MCL_ECP_dbl(P);
		MCL_ECP_add(P,&Q);
	}
	MCL_ECP_sub(P,&C); /* apply correction */
#endif
	MCL_ECP_affine(P);
}

#if MCL_CURVETYPE!=MCL_MONTGOMERY
/* Set P=eP+fQ double multiplication */
/* constant time - as useful for GLV method in pairings */
/* SU=456 */

void MCL_ECP_mul2(MCL_ECP *P,MCL_ECP *Q,MCL_BIG e,MCL_BIG f)
{
	mcl_chunk te[MCL_BS],tf[MCL_BS],mt[MCL_BS];
	MCL_ECP S,T,W[8],C;
	sign8 w[1+(MCL_NLEN*MCL_BASEBITS+1)/2];
	int i,a,b,s,ns,nb;
#if MCL_CURVETYPE==MCL_WEIERSTRASS
	mcl_chunk work[8][MCL_BS];
#endif

	MCL_ECP_affine(P);
	MCL_ECP_affine(Q);

	MCL_BIG_copy(te,e);
	MCL_BIG_copy(tf,f);

/* precompute table */
	MCL_ECP_copy(&W[1],P); MCL_ECP_sub(&W[1],Q);  /* P+Q */
	MCL_ECP_copy(&W[2],P); MCL_ECP_add(&W[2],Q);  /* P-Q */
	MCL_ECP_copy(&S,Q); MCL_ECP_dbl(&S);  /* S=2Q */ 
	MCL_ECP_copy(&W[0],&W[1]); MCL_ECP_sub(&W[0],&S); 
	MCL_ECP_copy(&W[3],&W[2]); MCL_ECP_add(&W[3],&S);
	MCL_ECP_copy(&T,P); MCL_ECP_dbl(&T); /* T=2P */
	MCL_ECP_copy(&W[5],&W[1]); MCL_ECP_add(&W[5],&T);
	MCL_ECP_copy(&W[6],&W[2]); MCL_ECP_add(&W[6],&T);
	MCL_ECP_copy(&W[4],&W[5]); MCL_ECP_sub(&W[4],&S);
	MCL_ECP_copy(&W[7],&W[6]); MCL_ECP_add(&W[7],&S);

#if MCL_CURVETYPE==MCL_WEIERSTRASS
	ECP_multiaffine(8,W,work);
#endif

/* if multiplier is odd, add 2, else add 1 to multiplier, and add 2P or P to correction */

	s=MCL_BIG_parity(te);
	MCL_BIG_inc(te,1); MCL_BIG_norm(te); ns=MCL_BIG_parity(te); MCL_BIG_copy(mt,te); MCL_BIG_inc(mt,1); MCL_BIG_norm(mt);
	MCL_BIG_cmove(te,mt,s);
	ECP_cmove(&T,P,ns);
	MCL_ECP_copy(&C,&T);

	s=MCL_BIG_parity(tf);
	MCL_BIG_inc(tf,1); MCL_BIG_norm(tf); ns=MCL_BIG_parity(tf); MCL_BIG_copy(mt,tf); MCL_BIG_inc(mt,1); MCL_BIG_norm(mt);
	MCL_BIG_cmove(tf,mt,s);
	ECP_cmove(&S,Q,ns);
	MCL_ECP_add(&C,&S);

	MCL_BIG_add(mt,te,tf); MCL_BIG_norm(mt);
	nb=1+(MCL_BIG_nbits(mt)+1)/2;

/* convert exponent to signed 2-bit window */
	for (i=0;i<nb;i++)
	{
		a=MCL_BIG_lastbits(te,3)-4;
		MCL_BIG_dec(te,a); MCL_BIG_norm(te); 
		MCL_BIG_fshr(te,2);
		b=MCL_BIG_lastbits(tf,3)-4;
		MCL_BIG_dec(tf,b); MCL_BIG_norm(tf); 
		MCL_BIG_fshr(tf,2);
		w[i]=4*a+b;
	}
	w[nb]=(4*MCL_BIG_lastbits(te,3)+MCL_BIG_lastbits(tf,3));

	MCL_ECP_copy(P,&W[(w[nb]-1)/2]);  
	for (i=nb-1;i>=0;i--)
	{
		ECP_select(&T,W,w[i]);
		MCL_ECP_dbl(P);
		MCL_ECP_dbl(P);
		MCL_ECP_add(P,&T);
	}
	MCL_ECP_sub(P,&C); /* apply correction */
	MCL_ECP_affine(P);
}

#endif

#ifdef HAS_MAIN

int main()
{
	int i;
	MCL_ECP G,P;
	csprng RNG;
	mcl_chunk r[MCL_BS],s[MCL_BS],x[MCL_BS],y[MCL_BS],b[MCL_BS],m[MCL_BS],w[MCL_BS],q[MCL_BS];
	MCL_BIG_rcopy(x,MCL_CURVE_Gx);
#if MCL_CURVETYPE!=MCL_MONTGOMERY
	MCL_BIG_rcopy(y,MCL_CURVE_Gy);
#endif
	MCL_BIG_rcopy(m,MCL_Modulus);

	printf("x= ");MCL_BIG_output(x); printf("\n");
#if MCL_CURVETYPE!=MCL_MONTGOMERY
	printf("y= ");MCL_BIG_output(y); printf("\n");
#endif
	MCL_RAND_seed(&RNG,3,"abc");

#if MCL_CURVETYPE!=MCL_MONTGOMERY
	MCL_ECP_set(&G,x,y);
#else
	MCL_ECP_set(&G,x);
#endif
	if (MCL_ECP_isinf(&G)) printf("Failed to set - point not on curve\n");
	else printf("set success\n");

	MCL_ECP_output(&G);

	MCL_ECP_copy(&P,&G);

	MCL_BIG_rcopy(r,MCL_CURVE_Order); //MCL_BIG_zero(r); MCL_BIG_inc(r,7);

	MCL_ECP_mul(&P,r);

	printf("rP= "); MCL_ECP_output(&P);

exit(0);


	MCL_BIG_rcopy(r,MCL_CURVE_Order); //MCL_BIG_dec(r,7);
	printf("r= ");MCL_BIG_output(r); printf("\n");

	MCL_ECP_copy(&P,&G);

	MCL_ECP_mul(&P,r);

	MCL_ECP_output(&P);
exit(0);	
	MCL_BIG_randomnum(w,r,&RNG);

	MCL_ECP_copy(&P,&G);
	MCL_ECP_mul(&P,w);

	MCL_ECP_output(&P);

	return 0;
}

#endif
