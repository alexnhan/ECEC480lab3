#ifndef _AES_H_
#define _AES_H_

#include <stdint.h>

// ECB enables the basic ECB 16-byte block algorithm. All can be enabled simultaneously.

#define AES128 1

#define AES_BLOCKLEN 16 //Block length in bytes AES is 128b block only

#define AES_KEYLEN 16   // Key length in bytes
#define AES_keyExpSize 176

struct AES_ctx
{
  uint8_t RoundKey[AES_keyExpSize];
};

void AES_init_ctx(struct AES_ctx* ctx, const uint8_t* key);
void AES_ECB_encrypt(struct AES_ctx* ctx, const uint8_t* buf);



#endif //_AES_H_
