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

/* Test C25519, C448, RSA2048 and RSA3072 */

#include "mcl_ecdh_runtime.h"
#include "mcl_rsa_runtime.h"
#include "mcl_utils.h"


static void testc25519()
{
  int res,i;
  char *pp="M0ng00se";
  char s0[MCL_EGS1],s1[MCL_EGS1],w0[2*MCL_EFS1+1],w1[2*MCL_EFS1+1],z0[MCL_EFS1];
  char z1[MCL_EFS1],seed[32],key[MCL_EAS],salt[32],pw[20],p1[30],p2[30];
  char v[2*MCL_EFS1+1],m[32],c[64],t[32],cs[MCL_EGS1],ds[MCL_EGS1];
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
  MCL_CREATE_CSPRNG_DREC1(&RNG,&SEED);   

  /* fake random salt value */
  char* saltHex = "7981eaa63589e7e4";
  MCL_hex2bin(saltHex, SALT.val, strlen(seedHex));
  SALT.len=8;				

  printf("CURVE 1 Alice's Passphrase= %s\r\n",pp);
  MCL_OCT_empty(&PW);
  // set Password from string
  MCL_OCT_jstring(&PW,pp);   

  /* private key S0 of size MCL_EGS1 bytes derived from Password and Salt */
  MCL_PBKDF2_DREC1(MCL_HASH_TYPE_ECC,&PW,&SALT,1000,MCL_EGS1,&S0);
  printf("CURVE 1 Alices private key= 0x");
  MCL_OCT_output(&S0);
  printf("\r\n");

  /* Generate Key pair S/W */
  MCL_ECP_KEY_PAIR_GENERATE_DREC1(NULL,&S0,&W0);
  res=MCL_ECP_PUBLIC_KEY_VALIDATE_DREC1(1,&W0);
  if (res!=0) {
    printf("MCL_DREC1_ECP Public Key is invalid!\r\n");
  }

  printf("CURVE 1 Alice's public key= 0x");  
  MCL_OCT_output(&W0);
  printf("\r\n");

  /* Random private key for other party */
  MCL_ECP_KEY_PAIR_GENERATE_DREC1(&RNG,&S1,&W1);

  res=MCL_ECP_PUBLIC_KEY_VALIDATE_DREC1(1,&W1);
  if (res!=0) {
    printf("MCL_DREC1_ECP Public Key is invalid!\r\n");
  }
  printf("CURVE 1 Servers private key= 0x");  
  MCL_OCT_output(&S1);
  printf("\r\n");
  
  printf("CURVE 1 Servers public key= 0x");   
  MCL_OCT_output(&W1);
  printf("\r\n");

  /* Calculate common key using DH - IEEE 1363 method */
  MCL_ECPSVDP_DH_DREC1(&S0,&W1,&Z0);
  MCL_ECPSVDP_DH_DREC1(&S1,&W0,&Z1);
   
  if (!MCL_OCT_comp(&Z0,&Z1)) {
    printf("*** MCL_DREC1_ECPSVDP-DH Failed\r\n");
  }

  MCL_KDF2_DREC1(MCL_HASH_TYPE_ECC,&Z0,NULL,MCL_EAS,&KEY);

  printf("CURVE 1 Alice's DH Key=  0x"); 
  MCL_OCT_output(&KEY);
  printf("\r\n");

  printf("CURVE 1 Servers DH Key=  0x"); 
  MCL_OCT_output(&KEY);
  printf("\r\n");

#if MCL_CURVETYPE!=MCL_MONTGOMERY

  printf("CURVE 1 Testing ECIES\r\n");

  P1.len=3; P1.val[0]=0x0; P1.val[1]=0x1; P1.val[2]=0x2; 
  P2.len=4; P2.val[0]=0x0; P2.val[1]=0x1; P2.val[2]=0x2; P2.val[3]=0x3; 

  M.len=17;
  for (i=0;i<=16;i++) M.val[i]=i; 

  MCL_ECP_ECIES_ENCRYPT_DREC1(MCL_HASH_TYPE_ECC,&P1,&P2,&RNG,&W1,&M,12,&V,&C,&T);

  printf("CURVE 1 Ciphertext: \r\n"); 
  printf("V= 0x"); MCL_OCT_output(&V);
  printf("C= 0x"); MCL_OCT_output(&C);
  printf("T= 0x"); MCL_OCT_output(&T);

  if (!MCL_ECP_ECIES_DECRYPT_DREC1(MCL_HASH_TYPE_ECC,&P1,&P2,&V,&C,&T,&S1,&M)) {
    printf("CURVE 1  ECIES Decryption Failed\r\n");
  } else {
    printf("CURVE 1 Decryption succeeded\r\n");
  }

  printf("CURVE 1 Message is 0x"); 
  MCL_OCT_output(&M);
  printf("\r\n");

  printf("CURVE 1 Testing ECDSA\r\n");

  if (MCL_ECPSP_DSA_DREC1(MCL_HASH_TYPE_ECC,&RNG,&S0,&M,&CS,&DS)!=0) {
    printf("CURVE 1 ECDSA Signature Failed\r\n");
  }

  printf("CURVE 1 Signature C = 0x"); 
  MCL_OCT_output(&CS);
  printf("\r\n");
  printf("CURVE 1 Signature D = 0x"); 
  MCL_OCT_output(&DS);
  printf("\r\n");

  if (MCL_ECPVP_DSA_DREC1(MCL_HASH_TYPE_ECC,&W0,&M,&CS,&DS)!=0) {
    printf("CURVE 1 ECDSA Verification Failed\r\n");
  } else {
    printf("CURVE 1 ECDSA Signature/Verification succeeded \r\n");
  }
#endif

  MCL_KILL_CSPRNG_DREC1(&RNG);
}

static void testc448()
{
  int res,i;
  char *pp="M0ng00se";
  char s0[MCL_EGS2],s1[MCL_EGS2],w0[2*MCL_EFS2+1],w1[2*MCL_EFS2+1],z0[MCL_EFS2];
  char z1[MCL_EFS2],seed[32],key[MCL_EAS],salt[32],pw[20],p1[30],p2[30];
  char v[2*MCL_EFS2+1],m[32],c[64],t[32],cs[MCL_EGS2],ds[MCL_EGS2];
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
  MCL_CREATE_CSPRNG_DREC2(&RNG,&SEED);   

  /* fake random salt value */
  char* saltHex = "7981eaa63589e7e4";
  MCL_hex2bin(saltHex, SALT.val, strlen(seedHex));
  SALT.len=8;				

  printf("CURVE 2 Alice's Passphrase= %s\r\n",pp);
  MCL_OCT_empty(&PW);
  // set Password from string
  MCL_OCT_jstring(&PW,pp);   

  /* private key S0 of size MCL_EGS2 bytes derived from Password and Salt */
  MCL_PBKDF2_DREC2(MCL_HASH_TYPE_ECC,&PW,&SALT,1000,MCL_EGS2,&S0);
  printf("CURVE 2 Alices private key= 0x");
  MCL_OCT_output(&S0);
  printf("\r\n");

  /* Generate Key pair S/W */
  MCL_ECP_KEY_PAIR_GENERATE_DREC2(NULL,&S0,&W0);
  res=MCL_ECP_PUBLIC_KEY_VALIDATE_DREC2(1,&W0);
  if (res!=0) {
    printf("MCL_DREC2_ECP Public Key is invalid!\r\n");
  }

  printf("CURVE 2 Alice's public key= 0x");  
  MCL_OCT_output(&W0);
  printf("\r\n");

  /* Random private key for other party */
  MCL_ECP_KEY_PAIR_GENERATE_DREC2(&RNG,&S1,&W1);

  res=MCL_ECP_PUBLIC_KEY_VALIDATE_DREC2(1,&W1);
  if (res!=0) {
    printf("MCL_DREC2_ECP Public Key is invalid!\r\n");
  }
  printf("CURVE 2 Servers private key= 0x");  
  MCL_OCT_output(&S1);
  printf("\r\n");
  
  printf("CURVE 2 Servers public key= 0x");   
  MCL_OCT_output(&W1);
  printf("\r\n");

  /* Calculate common key using DH - IEEE 1363 method */
  MCL_ECPSVDP_DH_DREC2(&S0,&W1,&Z0);
  MCL_ECPSVDP_DH_DREC2(&S1,&W0,&Z1);
   
  if (!MCL_OCT_comp(&Z0,&Z1)) {
    printf("*** MCL_DREC2_ECPSVDP-DH Failed\r\n");
  }

  MCL_KDF2_DREC2(MCL_HASH_TYPE_ECC,&Z0,NULL,MCL_EAS,&KEY);

  printf("CURVE 2 Alice's DH Key=  0x"); 
  MCL_OCT_output(&KEY);
  printf("\r\n");

  printf("CURVE 2 Servers DH Key=  0x"); 
  MCL_OCT_output(&KEY);
  printf("\r\n");

#if MCL_CURVETYPE!=MCL_MONTGOMERY

  printf("CURVE 2 Testing ECIES\r\n");

  P1.len=3; P1.val[0]=0x0; P1.val[1]=0x1; P1.val[2]=0x2; 
  P2.len=4; P2.val[0]=0x0; P2.val[1]=0x1; P2.val[2]=0x2; P2.val[3]=0x3; 

  M.len=17;
  for (i=0;i<=16;i++) M.val[i]=i; 

  MCL_ECP_ECIES_ENCRYPT_DREC2(MCL_HASH_TYPE_ECC,&P1,&P2,&RNG,&W1,&M,12,&V,&C,&T);

  printf("CURVE 2 Ciphertext: \r\n"); 
  printf("V= 0x"); MCL_OCT_output(&V);
  printf("C= 0x"); MCL_OCT_output(&C);
  printf("T= 0x"); MCL_OCT_output(&T);

  if (!MCL_ECP_ECIES_DECRYPT_DREC2(MCL_HASH_TYPE_ECC,&P1,&P2,&V,&C,&T,&S1,&M)) {
    printf("CURVE 2  ECIES Decryption Failed\r\n");
  } else {
    printf("CURVE 2 Decryption succeeded\r\n");
  }

  printf("CURVE 2 Message is 0x"); 
  MCL_OCT_output(&M);
  printf("\r\n");

  printf("CURVE 2 Testing ECDSA\r\n");

  if (MCL_ECPSP_DSA_DREC2(MCL_HASH_TYPE_ECC,&RNG,&S0,&M,&CS,&DS)!=0) {
    printf("CURVE 2 ECDSA Signature Failed\r\n");
  }

  printf("CURVE 2 Signature C = 0x"); 
  MCL_OCT_output(&CS);
  printf("\r\n");
  printf("CURVE 2 Signature D = 0x"); 
  MCL_OCT_output(&DS);
  printf("\r\n");

  if (MCL_ECPVP_DSA_DREC2(MCL_HASH_TYPE_ECC,&W0,&M,&CS,&DS)!=0) {
    printf("CURVE 2 ECDSA Verification Failed\r\n");
  } else {
    printf("CURVE 2 ECDSA Signature/Verification succeeded \r\n");
  }
#endif

  MCL_KILL_CSPRNG_DREC2(&RNG);
}

static void testrsa2048()
{
  char m[MCL_RFS1],ml[MCL_RFS1],c[MCL_RFS1],e[MCL_RFS1],s[MCL_RFS1],seed[32];

  MCL_rsa_public_key_DRRSA1 pub;
  MCL_rsa_private_key_DRRSA1 priv;
  csprng RNG;  
  mcl_octet M={0,sizeof(m),m};
  mcl_octet ML={0,sizeof(ml),ml};
  mcl_octet C={0,sizeof(c),c};
  mcl_octet E={0,sizeof(e),e};
  mcl_octet S={0,sizeof(s),s};
  mcl_octet SEED={0,sizeof(seed),seed};

  /* fake random seed source */
  char* seedHex = "d50f4137faff934edfa309c110522f6f5c0ccb0d64e5bf4bf8ef79d1fe21031a";
  MCL_hex2bin(seedHex, SEED.val, 64);
  SEED.len=32;				

  /* initialise strong RNG */
  MCL_RSA_CREATE_CSPRNG_DRRSA1(&RNG,&SEED);   

  printf("RSA2048 Generating public/private key pair\r\n");
  MCL_RSA_KEY_PAIR_DRRSA1(&RNG,65537,&priv,&pub);

  printf("RSA2048 Encrypting test string\r\n");
  MCL_OCT_jstring(&M,(char *)"Hello World\n");
  /* OAEP encode message m to e  */
  MCL_OAEP_ENCODE_DRRSA1(MCL_HASH_TYPE_RSA,&M,&RNG,NULL,&E); 

  /* encrypt encoded message */
  MCL_RSA_ENCRYPT_DRRSA1(&pub,&E,&C);     
  printf("RSA2048 Ciphertext= "); 
  MCL_OCT_output(&C); 
  printf("\r\n");

  printf("RSA2048 Decrypting test string\r\n");
  MCL_RSA_DECRYPT_DRRSA1(&priv,&C,&ML);  

  MCL_OAEP_DECODE_DRRSA1(MCL_HASH_TYPE_RSA,NULL,&ML);    /* decode it */
  MCL_OCT_output_string(&ML);
  printf("\r\n");

  MCL_OCT_clear(&M); MCL_OCT_clear(&ML);   /* clean up afterwards */
  MCL_OCT_clear(&C); MCL_OCT_clear(&SEED); MCL_OCT_clear(&E); 

  printf("RSA2048 Signing message\r\n");
  MCL_PKCS15_DRRSA1(MCL_HASH_TYPE_RSA,&M,&C);
  MCL_RSA_DECRYPT_DRRSA1(&priv,&C,&S); /* create signature in S */ 

  printf("RSA2048 Signature= "); 
  MCL_OCT_output(&S);
  printf("\r\n");

  MCL_RSA_ENCRYPT_DRRSA1(&pub,&S,&ML); 

  if (MCL_OCT_comp(&C,&ML)) {
    printf("RSA2048 Signature is valid\r\n");
  } else {
    printf("RSA2048 Signature is invalid\r\n");
  }

  MCL_RSA_KILL_CSPRNG_DRRSA1(&RNG);

  MCL_RSA_PRIVATE_KEY_KILL_DRRSA1(&priv);
}


static void testrsa3072()
{
  char m[MCL_RFS3],ml[MCL_RFS3],c[MCL_RFS3],e[MCL_RFS3],s[MCL_RFS3],seed[32];

  MCL_rsa_public_key_DRRSA3 pub;
  MCL_rsa_private_key_DRRSA3 priv;
  csprng RNG;  
  mcl_octet M={0,sizeof(m),m};
  mcl_octet ML={0,sizeof(ml),ml};
  mcl_octet C={0,sizeof(c),c};
  mcl_octet E={0,sizeof(e),e};
  mcl_octet S={0,sizeof(s),s};
  mcl_octet SEED={0,sizeof(seed),seed};

  /* fake random seed source */
  char* seedHex = "d50f4137faff934edfa309c110522f6f5c0ccb0d64e5bf4bf8ef79d1fe21031a";
  MCL_hex2bin(seedHex, SEED.val, 64);
  SEED.len=32;				

  /* initialise strong RNG */
  MCL_RSA_CREATE_CSPRNG_DRRSA3(&RNG,&SEED);   

  printf("RSA3072 Generating public/private key pair\r\n");
  MCL_RSA_KEY_PAIR_DRRSA3(&RNG,65537,&priv,&pub);

  printf("RSA3072 Encrypting test string\r\n");
  MCL_OCT_jstring(&M,(char *)"Hello World\n");
  /* OAEP encode message m to e  */
  MCL_OAEP_ENCODE_DRRSA3(MCL_HASH_TYPE_RSA,&M,&RNG,NULL,&E); 

  /* encrypt encoded message */
  MCL_RSA_ENCRYPT_DRRSA3(&pub,&E,&C);     
  printf("RSA3072 Ciphertext= "); 
  MCL_OCT_output(&C); 
  printf("\r\n");

  printf("RSA3072 Decrypting test string\r\n");
  MCL_RSA_DECRYPT_DRRSA3(&priv,&C,&ML);  

  MCL_OAEP_DECODE_DRRSA3(MCL_HASH_TYPE_RSA,NULL,&ML);    /* decode it */
  MCL_OCT_output_string(&ML);
  printf("\r\n");

  MCL_OCT_clear(&M); MCL_OCT_clear(&ML);   /* clean up afterwards */
  MCL_OCT_clear(&C); MCL_OCT_clear(&SEED); MCL_OCT_clear(&E); 

  printf("RSA3072 Signing message\r\n");
  MCL_PKCS15_DRRSA3(MCL_HASH_TYPE_RSA,&M,&C);
  MCL_RSA_DECRYPT_DRRSA3(&priv,&C,&S); /* create signature in S */ 

  printf("RSA3072 Signature= "); 
  MCL_OCT_output(&S);
  printf("\r\n");

  MCL_RSA_ENCRYPT_DRRSA3(&pub,&S,&ML); 

  if (MCL_OCT_comp(&C,&ML)) {
    printf("RSA3072 Signature is valid\r\n");
  } else {
    printf("RSA3072 Signature is invalid\r\n");
  }

  MCL_RSA_KILL_CSPRNG_DRRSA3(&RNG);

  MCL_RSA_PRIVATE_KEY_KILL_DRRSA3(&priv);
}

static void test()
{
  testc25519();
  testc448();
  testrsa2048();
  testrsa3072();
}

#ifdef MCL_BUILD_ARM
/* Thread handle */
static os_thread_t test_thread;
/* Buffer to be used as stack */
static os_thread_stack_define(test_stack, 16 * 1024);

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

