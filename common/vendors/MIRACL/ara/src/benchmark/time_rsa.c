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


/* Time RSA API Functions */

 
#include "mcl_rsa.h"
#include "mcl_utils.h"

const int nIter = ITERATIONS;

static void test()
{
  int i;
  char m[MCL_RFS],ml[MCL_RFS],c[MCL_RFS],e[MCL_RFS],seed[32];

  MCL_rsa_public_key pub;
  MCL_rsa_private_key priv;
  csprng RNG;  
  mcl_octet M={0,sizeof(m),m};
  mcl_octet ML={0,sizeof(ml),ml};
  mcl_octet C={0,sizeof(c),c};
  mcl_octet E={0,sizeof(e),e};
  mcl_octet SEED={0,sizeof(seed),seed};

  /* fake random seed source */
  char* seedHex = "d50f4137faff934edfa309c110522f6f5c0ccb0d64e5bf4bf8ef79d1fe21031a";
  MCL_hex2bin(seedHex, SEED.val, 64);
  SEED.len=32;				

#ifdef MCL_BUILD_ARM
  unsigned int t1;
#else
  double t1;
#endif			
  unsigned int totalTime;

  /* initialise strong RNG */
  MCL_RSA_CREATE_CSPRNG(&RNG,&SEED);   

  printf("Generating public/private key pair\r\n");
  t1 = MCL_start_time();
  for (i=0; i<nIter; i++) {
    printf("Iter %d\r\n", i);
    MCL_RSA_KEY_PAIR(&RNG,65537,&priv,&pub);
  }
  totalTime = MCL_end_time(t1);
  printf("MCL_RSA_KEY_PAIR: Iterations %d Total %d usecs Iteration %d usecs \r\n", nIter, totalTime, totalTime/nIter);

  printf("Encrypting test string\r\n");
  t1 = MCL_start_time();
  for (i=0; i<nIter; i++) {
    printf("Iter %d\r\n", i);
    MCL_OCT_jstring(&M,(char *)"Hello World\n");
    /* OAEP encode message m to e  */
    MCL_OAEP_ENCODE(MCL_HASH_TYPE_RSA,&M,&RNG,NULL,&E); 

    /* encrypt encoded message */
    MCL_RSA_ENCRYPT(&pub,&E,&C);     
  }
  totalTime = MCL_end_time(t1);
  printf("ENCRYPTION: Iterations %d Total %d usecs Iteration %d usecs \r\n", nIter, totalTime, totalTime/nIter);

  printf("Ciphertext= "); 
  MCL_OCT_output(&C); 
  printf("\r\n");

  printf("Decrypting test string\r\n");
  t1 = MCL_start_time();
  for (i=0; i<nIter; i++) {
    printf("Iter %d\r\n", i);
    MCL_RSA_DECRYPT(&priv,&C,&ML);  

    /* decode it */
    MCL_OAEP_DECODE(MCL_HASH_TYPE_RSA,NULL,&ML);   
  }
  totalTime = MCL_end_time(t1);
  printf("DECRYPTION: Iterations %d Total %d usecs Iteration %d usecs \r\n", nIter, totalTime, totalTime/nIter);

  printf("Plaintext= "); 
  MCL_OCT_output_string(&ML);
  printf("\r\n");
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
