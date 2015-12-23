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
#include "mcl_gcm.h"
#include "mcl_utils.h"

#define LINE_LEN 300

int main(int argc, char** argv)
{
  if (argc != 2) {
    printf("usage: ./testgcm_encrypt [path to test vector file]\n");
  }

  FILE * fp = NULL;
  char line[LINE_LEN];
  char * linePtr = NULL;
  int l1=0;
  char * Key = NULL;
  int KeyLen = 0;
  const char* KeyStr = "Key = ";
  char * IV = NULL;
  int IVLen = 0;
  const char* IVStr = "IV = ";
  char * PT = NULL;
  int PTLen = 0;
  const char* PTStr = "PT = ";
  char * AAD = NULL;
  int AADLen = 0;
  const char* AADStr = "AAD = ";
  char * CT = NULL;
  int CTLen = 0;        
  char * CTHex = NULL;
  const char* CTStr = "CT = ";
  char Tag[16];
  char * TagHex = NULL;
  const char* TagStr = "Tag = ";

  fp = fopen(argv[1], "r");
  if (fp == NULL)
      exit(EXIT_FAILURE);

  int i=0; 
  while (fgets(line, LINE_LEN, fp) != NULL)  {
    i++;
    if (!strncmp(line, KeyStr, strlen(KeyStr))) {
      // printf("line %d %s\n", i,line);	
      // Find hex value in string
      linePtr = line + strlen(KeyStr);

      // Allocate memory
      l1 = strlen(linePtr)-1;
      KeyLen = l1/2;
      Key = (char*) malloc (KeyLen);
      if (Key==NULL) 
        exit(EXIT_FAILURE);

      // Key binary value
      MCL_hex2bin(linePtr, Key, l1);
    }

    if (!strncmp(line, IVStr, strlen(IVStr))) {
      // printf("line %d %s\n", i,line);
      // Find hex value in string
      linePtr = line + strlen(IVStr);

      // Allocate memory
      l1 = strlen(linePtr)-1;
      IVLen = l1/2;
      IV = (char*) malloc (IVLen);
      if (IV==NULL) 
        exit(EXIT_FAILURE);

      // IV binary value
      MCL_hex2bin(linePtr, IV, l1);
    }

    if (!strncmp(line, PTStr, strlen(PTStr))) {
      // printf("line %d %s\n", i,line);
      // Find hex value in string
      linePtr = line + strlen(PTStr);

      // Allocate memory
      l1 = strlen(linePtr)-1;
      PTLen = l1/2;
      PT = (char*) malloc (PTLen);
      if (PT==NULL) 
        exit(EXIT_FAILURE);

      // PT binary value
      MCL_hex2bin(linePtr, PT, l1);
    }

    if (!strncmp(line, AADStr, strlen(AADStr))) {
      // printf("line %d %s\n", i,line);
      // Find hex value in string
      linePtr = line + strlen(AADStr);

      // Allocate memory
      l1 = strlen(linePtr)-1;
      AADLen = l1/2;
      AAD = (char*) malloc (AADLen);
      if (AAD==NULL) 
        exit(EXIT_FAILURE);

      // AAD binary value
      MCL_hex2bin(linePtr, AAD, l1);
    }

    if (!strncmp(line, CTStr, strlen(CTStr))) {
      // printf("line %d %s\n", i,line);
      // Find hex value in string
      linePtr = line + strlen(CTStr);

      // Allocate memory
      l1 = strlen(linePtr);
      CTHex = (char*) malloc (l1);
      if (CTHex==NULL) 
        exit(EXIT_FAILURE);

      CTLen = l1/2;
      CT = (char*) malloc (CTLen);
      if (CT==NULL) 
        exit(EXIT_FAILURE);

      // Golden CT value
      strcpy(CTHex, linePtr);
    }

    if (!strncmp(line, TagStr, strlen(TagStr))) {
      // printf("line %d %s\n", i,line);
      // Find hex value in string
      linePtr = line + strlen(TagStr);

      // Allocate memory
      l1 = strlen(linePtr);
      TagHex = (char*) malloc (l1);
      if (TagHex==NULL) 
        exit(EXIT_FAILURE);

      // Golden TAG value
      strcpy(TagHex, linePtr);

      mcl_gcm g;

      MCL_GCM_init(&g,KeyLen,Key,IVLen,IV);
      MCL_GCM_add_header(&g,AAD,AADLen);
      MCL_GCM_add_plain(&g,CT,PT,PTLen);
      MCL_GCM_finish(&g,Tag);
    
      int rc = MCL_test_value(CTHex, CT);
      if (rc) {
        printf("1 TEST GCM ENCRYPT FAILED LINE %d\n",i);
        exit(EXIT_FAILURE);
      }
    
      rc = MCL_test_value(TagHex, Tag);
      if (rc){
        printf("2 TEST GCM ENCRYPT FAILED LINE %d\n",i);
        exit(EXIT_FAILURE);
      }

      free(Key);
      free(IV);
      free(PT);
      free(AAD);
      free(CT);
      free(CTHex);
      free(TagHex);
    }
  }
  fclose(fp);
  printf("TEST GCM ENCRYPT PASSED\n");
  exit(EXIT_SUCCESS);
}
