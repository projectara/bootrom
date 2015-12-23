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
 *   Cryptographic strong random number generator 
 *
 *   Unguessable seed -> SHA -> PRNG internal state -> SHA -> random numbers
 *   Slow - but secure
 *
 *   See ftp://ftp.rsasecurity.com/pub/pdfs/bull-1.pdf for a justification
 */
/* SU=m, m is Stack Usage */

#include "mcl_arch.h"
#include "mcl_rand.h"
#include "mcl_hash.h"

/* SU= 20 */
static unsign32 sbrand(csprng *rng)
{ /* Marsaglia & Zaman random number generator */
    int i,k;
    unsign32 pdiff,t;
    rng->rndptr++;
    if (rng->rndptr<NK) return rng->ira[rng->rndptr];
    rng->rndptr=0;
    for (i=0,k=NK-NJ;i<NK;i++,k++)
    { /* calculate next NK values */
        if (k==NK) k=0;
        t=rng->ira[k];
        pdiff=t - rng->ira[i] - rng->borrow;

        if (pdiff<t) rng->borrow=0;
        if (pdiff>t) rng->borrow=1;
        rng->ira[i]=pdiff; 
    }
    return rng->ira[0];
}

/* SU= 20 */
static void sirand(csprng* rng,unsign32 seed)
{ /* initialise random number system */
  /* modified so that a subsequent call "stirs" in another seed value */
  /* in this way as many seed bits as desired may be used */
    int i,in;
    unsign32 t,m=1;
    rng->borrow=0L;
    rng->rndptr=0;
    rng->ira[0]^=seed;
    for (i=1;i<NK;i++)
    { /* fill initialisation vector */
        in=(NV*i)%NK;
        rng->ira[in]^=m;      /* note XOR */
        t=m;
        m=seed-m;
        seed=t;
    }
    for (i=0;i<10000;i++) sbrand(rng ); /* "warm-up" & stir the generator */
}

/* SU= 312 */
static void fill_pool(csprng *rng)
{ /* hash down output of RNG to re-fill the pool */
    int i;
    mcl_hash256 sh;
    MCL_HASH256_init(&sh);
    for (i=0;i<128;i++) MCL_HASH256_process(&sh,sbrand(rng));
    MCL_HASH256_hash(&sh,rng->pool);
    rng->pool_ptr=0;
}

static unsign32 pack(const uchar *b)
{ /* pack bytes into a 32-bit Word */
    return ((unsign32)b[3]<<24)|((unsign32)b[2]<<16)|((unsign32)b[1]<<8)|(unsign32)b[0];
}

/* SU= 360 */
/* Initialize RNG with some real entropy from some external source */
void MCL_RAND_seed(csprng *rng,int rawlen,char *raw)
{ /* initialise from at least 128 byte string of raw  *
   * random (keyboard?) input, and 32-bit time-of-day */
    int i;
    char digest[32];
    uchar b[4];
    mcl_hash256 sh;
    rng->pool_ptr=0;
    for (i=0;i<NK;i++) rng->ira[i]=0;
    if (rawlen>0)
    {
        MCL_HASH256_init(&sh);
        for (i=0;i<rawlen;i++)
            MCL_HASH256_process(&sh,raw[i]);
        MCL_HASH256_hash(&sh,digest);

/* initialise PRNG from distilled randomness */

        for (i=0;i<8;i++) 
		{
			b[0]=digest[4*i]; b[1]=digest[4*i+1]; b[2]=digest[4*i+2]; b[3]=digest[4*i+3];
		//	printf("%08x\n",pack(b));
			sirand(rng,pack(b));
		}
    }
    fill_pool(rng);
}

/* Terminate and clean up */
void MCL_RAND_clean(csprng *rng)
{ /* kill internal state */
    int i;
    rng->pool_ptr=rng->rndptr=0;
    for (i=0;i<32;i++) rng->pool[i]=0;
    for (i=0;i<NK;i++) rng->ira[i]=0;
    rng->borrow=0;
}

/* get random byte */
/* SU= 8 */
int MCL_RAND_byte(csprng *rng)
{ 
    int r;
    r=rng->pool[rng->pool_ptr++];
    if (rng->pool_ptr>=32) fill_pool(rng);
    return (r&0xff);
}

/* test main program */
/*
#include <stdio.h>
#include <string.h>

void main()
{
    int i;
    char raw[256];
    csprng rng;
 
	MCL_RAND_clean(&rng);


	for (i=0;i<256;i++) raw[i]=(char)i;
    MCL_RAND_seed(&rng,256,raw);
 
	for (i=0;i<1000;i++)
		printf("%02x ",(unsigned char)MCL_RAND_byte(&rng));
}

*/
