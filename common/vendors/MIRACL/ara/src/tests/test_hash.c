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

#include "mcl_arch.h"
#include "mcl_hash.h"
#include "mcl_utils.h"

/* test program using NIST vectors */

static void test()
{
  char digest[64];
  int i;
  mcl_hash160 sh160;
  mcl_hash256 sh256;
  mcl_hash384 sh384;
  mcl_hash512 sh512;

  char* Msg160Hex = "9597f714b2e45e3399a7f02aec44921bd78be0fefee0c5e9b499488f6e";
  char Msg160[29];
  char* MD160Hex = "b61ce538f1a1e6c90432b233d7af5b6524ebfbe3";
  MCL_hex2bin(Msg160Hex,Msg160,58);

  char* Msg256Hex = "1a57251c431d4e6c2e06d65246a296915071a531425ecf255989422a66";
  char Msg256[29];
  char* MD256Hex = "c644612cd326b38b1c6813b1daded34448805aef317c35f548dfb4a0d74b8106";
  MCL_hex2bin(Msg256Hex,Msg256,58);

  char* Msg384Hex = "718e0cfe1386cb1421b4799b15788b862bf03a8072bb30d02303888032";
  char Msg384[29];
  char* MD384Hex = "6d8b8a5bc7ea365ea07f11d3b12e95872a9633684752495cc431636caf1b273a35321044af31c974d8575d38711f56c6";
  MCL_hex2bin(Msg384Hex,Msg384,58);

  char* Msg512Hex = "ab1fc9ee845eeb205ec13725daf1fb1f5d50629b14ea9a2235a9350a88";
  char Msg512[29];
  char* MD512Hex = "72d188a9df5f3b00057bca22c92c0f8228422d974302d22d4b322e7a6c8fc3b2b50ec74c6842781f29f7075c3d4bd065878648846c39bb3e4e2692c0f053f7ed";
  MCL_hex2bin(Msg512Hex,Msg512,58);

  MCL_HASH160_init(&sh160);
  for (i=0;i<29;i++) 
    MCL_HASH160_process(&sh160,Msg160[i]);
  MCL_HASH160_hash(&sh160,digest); 
  printf("Want %s \r\n", MD160Hex);   
  printf("Got  ");
  for (i=0;i<20;i++) 
    printf("%02x",(unsigned char)digest[i]);
  printf("\r\n");

  MCL_HASH256_init(&sh256);
  for (i=0;i<29;i++) 
    MCL_HASH256_process(&sh256,Msg256[i]);
  MCL_HASH256_hash(&sh256,digest); 
  printf("Want %s \r\n", MD256Hex);   
  printf("Got  ");
  for (i=0;i<32;i++) 
    printf("%02x",(unsigned char)digest[i]);
  printf("\r\n");

  MCL_HASH384_init(&sh384);
  for (i=0;i<29;i++) 
    MCL_HASH384_process(&sh384,Msg384[i]);
  MCL_HASH384_hash(&sh384,digest); 
  printf("Want %s \r\n", MD384Hex);   
  printf("Got  ");
  for (i=0;i<48;i++) 
    printf("%02x",(unsigned char)digest[i]);
  printf("\r\n");

  MCL_HASH512_init(&sh512);
  for (i=0;i<29;i++) 
    MCL_HASH512_process(&sh512,Msg512[i]);
  MCL_HASH512_hash(&sh512,digest); 
  printf("Want %s \r\n", MD512Hex);   
  printf("Got  ");
  for (i=0;i<64;i++) 
    printf("%02x",(unsigned char)digest[i]);
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
