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


/*** Basic Octet string maintainance routines  ***/
/* SU=m, m is Stack Usage */

#include "mcl_oct.h"

/* Output an mcl_octet string (Debug Only) */

#ifdef MCL_BUILD_TEST
/* SU= 16 */
/* output mcl_octet */
void MCL_OCT_output(mcl_octet *w)
{
    int i;
    unsigned char ch;
    for (i=0;i<w->len;i++)
    {
        ch=w->val[i];
        printf("%02x",ch);
    }
    printf("\n");
}  

/* SU= 16 */
void MCL_OCT_output_string(mcl_octet *w)
{
    int i;
    unsigned char ch;
    for (i=0;i<w->len;i++)
    {
        ch=w->val[i];
        printf("%c",ch);
    }
  /*  printf("\n"); */
} 
#endif // MCL_BUILD_TEST

/* Convert C string to mcl_octet format - truncates if no room  */
void MCL_OCT_jstring(mcl_octet *y,char *s)
{
    int i,j;
    i=y->len;
    j=0;
    while (s[j]!=0 && i<y->max)
    {
        y->val[i]=s[j];
        y->len++;
        i++;  j++;
    }
}

/* compare 2 mcl_octet strings. 
 * If x==y return TRUE, else return FALSE */ 
/* SU= 8 */
int MCL_OCT_comp(mcl_octet *x,mcl_octet *y)
{
    int i;
    if (x->len>y->len) return 0;
    if (x->len<y->len) return 0;
    for (i=0;i<x->len;i++)
    {
        if (x->val[i]!=y->val[i]) return 0;
    }
    return 1;
}

/* check are first n bytes the same */

int MCL_OCT_ncomp(mcl_octet *x,mcl_octet *y,int n)
{
    int i;
    if (n>y->len || n>x->len) return 0;
    for (i=0;i<n;i++)
    {
        if (x->val[i]!=y->val[i]) return 0;
    }
    return 1;
}

/* Shift mcl_octet to the left by n bytes. Leftmost bytes disappear  */
void MCL_OCT_shl(mcl_octet *x,int n)
{
    int i;
    if (n>=x->len)
    {
        x->len=0;
        return;
    }
    x->len-=n;
    for (i=0;i<x->len;i++)
        x->val[i]=x->val[i+n];
} 

/* Shift mcl_octet to the right by n bytes. LSBs become 0 */
void MCL_OCT_shr(mcl_octet *x,int n)
{
	int i;
	if (n==0 || x->len+n>x->max) return;

	for (i=x->len+n-1;i>=n;i--) x->val[i]=x->val[i-n];
	for (i=0;i<n;i++) x->val[i]=0;
}

/* Append binary string to mcl_octet - truncates if no room */
/* SU= 12 */
void MCL_OCT_jbytes(mcl_octet *y,char *b,int len)
{
    int i,j;
    i=y->len;
    for (j=0;j<len && i<y->max;j++) 
    {
        y->val[i]=b[j];
        y->len++;
        i++;
    }
}

/* Concatenates two mcl_octet strings */
/* SU= 8 */
void MCL_OCT_jmcl_octet(mcl_octet *y,mcl_octet *x)
{ /* y=y || x */
    int i,j;
    if (x==NULL) return;

    for (i=0;i<x->len;i++)
    {
        j=y->len+i;
        if (j>=y->max)
        {
            y->len=y->max;
            return;
        }
        y->val[j]=x->val[i];
    }
    y->len+=x->len;
}

/* Append byte to mcl_octet rep times */
/* SU= 8 */
void MCL_OCT_jbyte(mcl_octet *y,int ch,int rep)
{
    int i,j;
    i=y->len;
    for (j=0;j<rep && i<y->max;j++) 
    {
        y->val[i]=ch;
        y->len++;
        i++;
    }
}

/* XOR common bytes of x with y */
/* SU= 8 */
void MCL_OCT_xor(mcl_octet *y,mcl_octet *x)
{ /* xor first x->len bytes of y */

    int i;
    for (i=0;i<x->len && i<y->len;i++)
    {
        y->val[i]^=x->val[i];
    }
}

/* clear an mcl_octet */
void MCL_OCT_empty(mcl_octet *w)
{
    w->len=0;
}

/* Kill an mcl_octet string - Zeroise it for security */
void MCL_OCT_clear(mcl_octet *w)
{
    int i;
    for (i=0;i<w->max;i++) w->val[i]=0;
    w->len=0;
}

/* appends int x of length len bytes to OCTET string */
/* SU= 8 */
void MCL_OCT_jint(mcl_octet *y,int x,int len) 
{
    int i,n;
    n=y->len+len;
    if (n>y->max || len<=0) return;
    for (i=y->len;i<n;i++) y->val[i]=0;
    y->len=n;

    i=y->len; 
    while (x>0 && i>0)
    {
        i--;
        y->val[i]=x%256;
        x/=256;
    }
}

/* Pad an mcl_octet to a given length */
/* SU= 8 */
int MCL_OCT_pad(mcl_octet *w,int n)
{
	int i,d;
	if (w->len>n || n>w->max) return 0;
	if (n==w->len) return 1;
	d=n-w->len;
	for (i=n-1;i>=d;i--)
		w->val[i]=w->val[i-d];
	for (i=d-1;i>=0;i--)
		w->val[i]=0;
	w->len=n;
	return 1;
}


/* Convert an mcl_octet string to base64 string */
/* SU= 56 */
void MCL_OCT_tobase64(char *b,mcl_octet *w)
{
	int i,j,k,rem,last;
	int c,ch[4];
	unsigned char ptr[3];
	rem=w->len%3; j=k=0; last=4;
	while (j<w->len)
	{
		for (i=0;i<3;i++)
		{
			if (j<w->len) ptr[i]=w->val[j++];
			else {ptr[i]=0; last--;}
		}
		ch[0]=(ptr[0]>>2)&0x3f;
		ch[1]=((ptr[0]<<4)|(ptr[1]>>4))&0x3f;
		ch[2]=((ptr[1]<<2)|(ptr[2]>>6))&0x3f;
		ch[3]=ptr[2]&0x3f;
		for (i=0;i<last;i++)
		{
			c=ch[i];
			if (c<26) c+=65;
            if (c>=26 && c<52) c+=71;
            if (c>=52 && c<62) c-=4;
            if (c==62) c='+';
            if (c==63) c='/';
			b[k++]=c;
		}
	}
	if (rem>0) for (i=rem;i<3;i++) b[k++]='=';
	b[k]='\0';  /* dangerous! */
}

#ifdef MCL_BUILD_TEST
/* SU= 56 */
void MCL_OCT_frombase64(mcl_octet *w,char *b)
{
	int i,j,k,pads,len=(int)strlen(b);
	int c,ch[4],ptr[3];
//	int lead=1;
	j=k=0;
	while (j<len && k<w->max)
	{
		pads=0;
		for (i=0;i<4;i++)
		{
			c=80+b[j++];
			if (c<=112) continue; /* ignore white space */
            if (c>144 && c<171) c-=145;
            if (c>176 && c<203) c-=151;
            if (c>127 && c<138) c-=76;
            if (c==123) c=62;
            if (c==127) c=63;
            if (c==141) {pads++; continue;} /* ignore pads '=' */
			ch[i]=c;
		}
		ptr[0]=(ch[0]<<2)|(ch[1]>>4);
		ptr[1]=(ch[1]<<4)|(ch[2]>>2);
		ptr[2]=(ch[2]<<6)|ch[3];
		for (i=0;i<3-pads && k<w->max;i++)
		{ /* don't put in leading zeros */
			/* if (lead && ptr[i]==0) continue; */
			w->val[k++]=ptr[i];
//			lead=0;
		}
		
	}
	w->len=k;
}
#endif // MCL_BUILD_TEST

/* copy an mcl_octet string - truncates if no room */
/* SU= 16 */
void MCL_OCT_copy(mcl_octet *y,mcl_octet *x)
{
    int i;
    MCL_OCT_clear(y);
    y->len=x->len;
    if (y->len>y->max) y->len=y->max;

    for (i=0;i<y->len;i++)
        y->val[i]=x->val[i];
}

/* XOR m with all of x */
void MCL_OCT_xorbyte(mcl_octet *x,int m)
{
    int i;
    for (i=0;i<x->len;i++) x->val[i]^=m;
}

/* truncates x to n bytes and places the rest in y (if y is not NULL) */
/* SU= 8 */
void MCL_OCT_chop(mcl_octet *x,mcl_octet *y,int n)
{
    int i;
    if (n>=x->len)
    {
        if (y!=NULL) y->len=0;
        return;
    }
    if (y!=NULL) y->len=x->len-n;
    x->len=n; 

    if (y!=NULL)
    {
        for (i=0;i<y->len && i<y->max;i++) y->val[i]=x->val[i+n];
    }
}

/* set x to len random bytes */
void MCL_OCT_rand(mcl_octet *x,csprng *RNG,int len)
{
    int i;
    if (len>x->max) len=x->max;
    x->len=len;

    for (i=0;i<len;i++) x->val[i]=MCL_RAND_byte(RNG);
}

#ifdef MCL_BUILD_TEST
/* Convert an mcl_octet to a hex string */
void MCL_OCT_toHex(mcl_octet *src,char *dst)
{
    int i;
    unsigned char ch;
    for (i=0;i<src->len;i++)
    {
        ch=src->val[i];
        sprintf(&dst[i*2],"%02x", ch);
    }
}

/* Convert an mcl_octet to a string */
void MCL_OCT_toStr(mcl_octet *src,char *dst)
{
    int i;
    unsigned char ch;
    for (i=0;i<src->len;i++)
    {
        ch=src->val[i];
        sprintf(&dst[i],"%c", ch);
    }
}
#endif // MCL_BUILD_TEST

/* Test program 
#include <stdio.h>
#include "aracrypt.h"

char test[]="abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

int main()
{
	char gm[100],gn[100],t[100];
    mcl_octet m={0,sizeof(gm),gm};
    mcl_octet n={0,sizeof(gn),gn};

	MCL_OCT_jbytes(&m,test,strlen(test));
	MCL_OCT_output(&m);

	MCL_OCT_tobase64(t,&m);
	printf(t); printf("\n");

	MCL_OCT_frombase64(&n,t);
	MCL_OCT_output(&n);

    return 0;
}
*/
