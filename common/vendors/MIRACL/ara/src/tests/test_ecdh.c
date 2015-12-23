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

/* ECDH smoke test */

#include "mcl_ecdh.h"
#include "mcl_utils.h"

static void test()
{
  int res,i;
  char *pp="M0ng00se";
  char s0[MCL_EGS],s1[MCL_EGS],w0[2*MCL_EFS+1],w1[2*MCL_EFS+1],z0[MCL_EFS],z1[MCL_EFS],seed[32],key[MCL_EAS],salt[32],pw[20],p1[30],p2[30],v[2*MCL_EFS+1],m[32],c[64],t[32],cs[MCL_EGS],ds[MCL_EGS];
  mcl_octet S0={0,sizeof(s0),s0};
  mcl_octet S1={0,sizeof(s1),s1};
  mcl_octet W0={0,sizeof(w0),w0};
  mcl_octet W1={0,sizeof(w1),w1};
  mcl_octet Z0={0,sizeof(z0),z0};
  mcl_octet Z1={0,sizeof(z1),z1};
  mcl_octet SEED={0,sizeof(seed),seed};
  mcl_octet KEY={0,sizeof(key),key};
  mcl_octet SALT={0,sizeof(salt),salt};
  mcl_octet PW={0,sizeof(pw),pw};
  mcl_octet P1={0,sizeof(p1),p1};
  mcl_octet P2={0,sizeof(p2),p2};
  mcl_octet V={0,sizeof(v),v};
  mcl_octet M={0,sizeof(m),m};
  mcl_octet C={0,sizeof(c),c};
  mcl_octet T={0,sizeof(t),t};
  mcl_octet CS={0,sizeof(cs),cs};
  mcl_octet DS={0,sizeof(ds),ds};
  csprng RNG;                

  /* fake random seed source */
  char* seedHex = "d50f4137faff934edfa309c110522f6f5c0ccb0d64e5bf4bf8ef79d1fe21031a";
  MCL_hex2bin(seedHex, SEED.val, 64);
  SEED.len=32;				

  /* initialise strong RNG */
  MCL_CREATE_CSPRNG(&RNG,&SEED);   

  /* fake random salt value */
  char* saltHex = "7981eaa63589e7e4";
  MCL_hex2bin(saltHex, SALT.val, strlen(seedHex));
  SALT.len=8;				

  printf("Alice's Passphrase= %s\r\n",pp);
  MCL_OCT_empty(&PW);
  // set Password from string
  MCL_OCT_jstring(&PW,pp);   

  /* private key S0 of size MCL_EGS bytes derived from Password and Salt */
  MCL_PBKDF2(MCL_HASH_TYPE_ECC,&PW,&SALT,1000,MCL_EGS,&S0);
  printf("Alices private key= 0x");
  MCL_OCT_output(&S0);
  printf("\r\n");

  /* Generate Key pair S/W */
  MCL_ECP_KEY_PAIR_GENERATE(NULL,&S0,&W0);
  res=MCL_ECP_PUBLIC_KEY_VALIDATE(1,&W0);
  if (res!=0) {
    printf("MCL_ECP Public Key is invalid!\r\n");
  }

  printf("Alice's public key= 0x");  
  MCL_OCT_output(&W0);
  printf("\r\n");

  /* Random private key for other party */
  MCL_ECP_KEY_PAIR_GENERATE(&RNG,&S1,&W1);

  res=MCL_ECP_PUBLIC_KEY_VALIDATE(1,&W1);
  if (res!=0) {
    printf("MCL_ECP Public Key is invalid!\r\n");
  }
  printf("Servers private key= 0x");  
  MCL_OCT_output(&S1);
  printf("\r\n");
  
  printf("Servers public key= 0x");   
  MCL_OCT_output(&W1);
  printf("\r\n");

  /* Calculate common key using DH - IEEE 1363 method */
  MCL_ECPSVDP_DH(&S0,&W1,&Z0);
  MCL_ECPSVDP_DH(&S1,&W0,&Z1);
   
  if (!MCL_OCT_comp(&Z0,&Z1)) {
    printf("*** MCL_ECPSVDP-DH Failed\r\n");
  }

  MCL_KDF2(MCL_HASH_TYPE_ECC,&Z0,NULL,MCL_EAS,&KEY);

  printf("Alice's DH Key=  0x"); 
  MCL_OCT_output(&KEY);
  printf("\r\n");

  printf("Servers DH Key=  0x"); 
  MCL_OCT_output(&KEY);
  printf("\r\n");

#if MCL_CURVETYPE!=MCL_MONTGOMERY

  printf("Testing ECIES\r\n");

  P1.len=3; P1.val[0]=0x0; P1.val[1]=0x1; P1.val[2]=0x2; 
  P2.len=4; P2.val[0]=0x0; P2.val[1]=0x1; P2.val[2]=0x2; P2.val[3]=0x3; 

  M.len=17;
  for (i=0;i<=16;i++) M.val[i]=i; 

  MCL_ECP_ECIES_ENCRYPT(MCL_HASH_TYPE_ECC,&P1,&P2,&RNG,&W1,&M,12,&V,&C,&T);

  printf("Ciphertext: \r\n"); 
  printf("V= 0x"); MCL_OCT_output(&V);
  printf("C= 0x"); MCL_OCT_output(&C);
  printf("T= 0x"); MCL_OCT_output(&T);

  if (!MCL_ECP_ECIES_DECRYPT(MCL_HASH_TYPE_ECC,&P1,&P2,&V,&C,&T,&S1,&M)) {
    printf("*** ECIES Decryption Failed\r\n");
  } else {
    printf("Decryption succeeded\r\n");
  }

  printf("Message is 0x"); 
  MCL_OCT_output(&M);
  printf("\r\n");

  printf("Testing ECDSA\r\n");

  if (MCL_ECPSP_DSA(MCL_HASH_TYPE_ECC,&RNG,&S0,&M,&CS,&DS)!=0) {
    printf("***ECDSA Signature Failed\r\n");
  }

  printf("Signature C = 0x"); 
  MCL_OCT_output(&CS);
  printf("\r\n");
  printf("Signature D = 0x"); 
  MCL_OCT_output(&DS);
  printf("\r\n");

  if (MCL_ECPVP_DSA(MCL_HASH_TYPE_ECC,&W0,&M,&CS,&DS)!=0) {
    printf("***ECDSA Verification Failed\r\n");
  } else {
    printf("ECDSA Signature/Verification succeeded \r\n");
  }
#endif

  MCL_KILL_CSPRNG(&RNG);
}

#ifdef MCL_BUILD_ARM
/* Thread handle */
static os_thread_t test_thread;
/* Buffer to be used as stack */
static os_thread_stack_define(test_stack, 8 * 1024);

/* create shadow yield thread */
static int create_test_thread()
{
	int ret;
	ret = os_thread_create(
		/* thread handle */
		&test_thread,
		/* thread name */
		"test",
		/* entry function */
		test,
		/* argument */
		0,
		/* stack */
		&test_stack,
		/* priority */
		OS_PRIO_3);
	if (ret != WM_SUCCESS) {
		wmprintf("Failed to create shadow yield thread: %d\r\n", ret);
		return -WM_FAIL;
	}
	return WM_SUCCESS;
}
#endif

int main()
{   
#ifdef MCL_BUILD_ARM
  /* Initialize console on uart0 */
  wmstdio_init(UART0_ID, 0);
#endif

#ifdef MCL_BUILD_ARM
  create_test_thread();
#else
  test();
#endif
  
  return 0;
}

