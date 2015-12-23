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

/* test driver and function exerciser for RSA API Functions */

 
#include "mcl_rsa.h"
#include "mcl_utils.h"

static void test()
{
  char m[MCL_RFS],ml[MCL_RFS],c[MCL_RFS],e[MCL_RFS],s[MCL_RFS],seed[32];

  MCL_rsa_public_key pub;
  MCL_rsa_private_key priv;
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
  MCL_RSA_CREATE_CSPRNG(&RNG,&SEED);   

  printf("Generating public/private key pair\r\n");
  MCL_RSA_KEY_PAIR(&RNG,65537,&priv,&pub);

  printf("Encrypting test string\r\n");
  MCL_OCT_jstring(&M,(char *)"Hello World\n");
  /* OAEP encode message m to e  */
  MCL_OAEP_ENCODE(MCL_HASH_TYPE_RSA,&M,&RNG,NULL,&E); 

  /* encrypt encoded message */
  MCL_RSA_ENCRYPT(&pub,&E,&C);     
  printf("Ciphertext= "); 
  MCL_OCT_output(&C); 
  printf("\r\n");

  printf("Decrypting test string\r\n");
  MCL_RSA_DECRYPT(&priv,&C,&ML);  

  MCL_OAEP_DECODE(MCL_HASH_TYPE_RSA,NULL,&ML);    /* decode it */
  MCL_OCT_output_string(&ML);
  printf("\r\n");

  MCL_OCT_clear(&M); MCL_OCT_clear(&ML);   /* clean up afterwards */
  MCL_OCT_clear(&C); MCL_OCT_clear(&SEED); MCL_OCT_clear(&E); 

  printf("Signing message\r\n");
  MCL_PKCS15(MCL_HASH_TYPE_RSA,&M,&C);
  MCL_RSA_DECRYPT(&priv,&C,&S); /* create signature in S */ 

  printf("Signature= "); 
  MCL_OCT_output(&S);
  printf("\r\n");

  MCL_RSA_ENCRYPT(&pub,&S,&ML); 

  if (MCL_OCT_comp(&C,&ML)) {
    printf("Signature is valid\r\n");
  } else {
    printf("Signature is INVALID\r\n");
  }

  MCL_RSA_KILL_CSPRNG(&RNG);

  MCL_RSA_PRIVATE_KEY_KILL(&priv);
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
