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


/* Functions to time code and deal with hex encoding / decoding */

#include "mcl_utils.h"

 
#ifdef MCL_BUILD_ARM
unsigned int MCL_start_time() 
{
  /* time in micro-secs since bootup */
  unsigned int t1 = os_get_timestamp();
  return t1;
}

unsigned int MCL_end_time(unsigned int t1)
{
  /* time in micro-secs since bootup */
  unsigned int t2 = os_get_timestamp();
  unsigned int totalTime = t2 - t1;
  return totalTime;
}
#else
double MCL_start_time() 
{
  struct timeval startTime;
  gettimeofday(&startTime, NULL);
  double t1 = startTime.tv_sec * 1000000 + startTime.tv_usec;
  return t1;
}

double MCL_end_time(double t1)
{
  struct timeval endTime;
  gettimeofday(&endTime, NULL);
  double t2 = endTime.tv_sec * 1000000 + endTime.tv_usec;
  double totalTime = t2 - t1;
  return totalTime;
}
#endif


#ifdef MCL_BUILD_TEST
void MCL_print_hex(char *bin, int len)
{
  int i;
  for (i = 0; i < len; i++) {
    printf("%02x", (unsigned char) bin[i]);
  }
  printf("\n");
}


void MCL_hex2bin(char *src, char *dst, int src_len)
{
  int i;
  char v;
  for (i = 0; i < src_len/2; i++) {
    char c = src[2*i];
    if (c >= '0' && c <= '9') {
        v = c - '0';
    } else if (c >= 'A' && c <= 'F') {
        v = c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        v = c - 'a' + 10;
    } else {
        v = 0;
    }
    v <<= 4;
    c = src[2*i + 1];
    if (c >= '0' && c <= '9') {
        v += c - '0';
    } else if (c >= 'A' && c <= 'F') {
        v += c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        v += c - 'a' + 10;
    } else {
        v = 0;
    }
    dst[i] = v;
  }
}

void MCL_bin2hex(char *src, char *dst, int src_len)
{
  int i;
  for (i = 0; i < src_len; i++) {
    sprintf(&dst[i*2],"%02x", (unsigned char) src[i]);
  }
}

#ifndef MCL_BUILD_ARM
int MCL_test_value(char * want, char * got)
{
  int l = strlen(want);
  char* buffer = (char*) malloc (l);
  if (buffer==NULL){ 
    free (buffer);
    return 1;
  }
  l=l/2;
  MCL_bin2hex(got, buffer, l);
  int rc = strncmp(want,buffer,l);
  if (rc) {
    printf("ERROR want: %s got: %s\n", want, buffer);
  }
  /* printf("DEBUG want: %s got: %s\n", want, buffer); */
  free(buffer);
  return rc;
}
#endif

#endif
