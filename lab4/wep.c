//Maureen Burnside
//ECEC T480 Feb 6 2018
//WEP Encryption

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

//functions to compute crc
uint32_t crc32_for_byte(uint32_t r) {
  for(int j = 0; j < 8; ++j)
    r = (r & 1? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
  return r ^ (uint32_t)0xFF000000L;
}

void crc32(const void *data, size_t n_bytes, uint32_t* crc) {
  static uint32_t table[0x100];
  if(!*table)
    for(size_t i = 0; i < 0x100; ++i)
      table[i] = crc32_for_byte(i);
  for(size_t i = 0; i < n_bytes; ++i)
    *crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
}

int main(int argc, char *argv[]){
    uint32_t checksum=0;

    unsigned char plaintextCRC[256];

	//add message to plaintext array
     plaintextCRC[0] = 'h';
     plaintextCRC[1] = 'e';
     plaintextCRC[2] = 'l';
     plaintextCRC[3] = 'l';
     plaintextCRC[4] = 'o';

	//pad plaintext array
   int i = 0;
    for(i = 5; i < 252; i++){
        plaintextCRC[i] = '\0';    
    }

	//calculate crc and add to plaintext array
    crc32(plaintextCRC, 252, &checksum);
    plaintextCRC[252] = checksum>>24 & 0x000000FF;
    plaintextCRC[253] = checksum>>16 & 0x000000FF;
    plaintextCRC[254] = checksum>>8 & 0x000000FF;
    plaintextCRC[255] = checksum>>0 & 0x000000FF;

	//populate key and iv array
    unsigned char keyiv[4];
    keyiv[0] = '\0';
    keyiv[1] = 'E';
    keyiv[2] = 'C';
    keyiv[3] = 'E';

	//compute RC4
    unsigned char S[256];
    int keylength = 4;
    for(i = 0; i < 256; i++){
        S[i] = i;
    }
    int j = 0;
    int val = 0;
    for(i = 0; i < 256; i++){
        j = (j + S[i] + keyiv[i % keylength]) % 256;
        val = S[i];
        S[i] = S[j];
        S[j] = val;
    } 

    unsigned char ciphertext[257];
	
	//populate ciphertext with iv and plaintext xor rc4
    ciphertext[0] = '\0';
    for(i = 1; i < 257; i++){
    ciphertext[i] = plaintextCRC[i-1]^S[i-1];
    }

	//send to output
   write(1,ciphertext,257);

   	return 0;
}

