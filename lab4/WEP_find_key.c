#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void RC4(unsigned char* key, unsigned int keylength, unsigned char* S);
void crc32(const void* data, size_t n_bytes, uint32_t* crc);
char is_identical(unsigned char* a, unsigned char* b, unsigned int n);
char *key_alph = "ABCDE";

int main() {
    unsigned char challenge[256];
    unsigned char c[256];
    unsigned char cf[257];
    unsigned char rc4[256];
    unsigned char rc4_test[256];
    unsigned char iv_key[5];
    unsigned int keylength=4;
    unsigned int checksum;
    int i;
    for(i=0;i<256;i++) {
        challenge[i] = 0;
    }
    challenge[0] = 'h';
    challenge[1] = 'e';
    challenge[2] = 'l';
    challenge[3] = 'l';
    challenge[4] = 'o';
//    read(0,challenge,252);
    read(0,cf,257);
    
    iv_key[0] = cf[0];
    for(i=0;i<256;i++) {
        c[i] = cf[i+1];
    }
    crc32(challenge,252,&checksum);
    challenge[252] = checksum>>24 & 0x000000FF;
    challenge[253] = checksum>>16 & 0x000000FF;
    challenge[254] = checksum>>8 & 0x000000FF;
    challenge[255] = checksum & 0x000000FF;

    // computing the rc4
    for(i=0;i<256;i++) {
        rc4[i] = challenge[i] ^ c[i];
    }
    
    int j,k;
    int n_letters = strlen(key_alph);
    for(i=0;i<n_letters;i++){
        for(j=0;j<n_letters;j++) {
            for(k=0;k<n_letters;k++) {
                iv_key[1] = key_alph[i];
                iv_key[2] = key_alph[j];
                iv_key[3] = key_alph[k];
                RC4(iv_key,keylength,rc4_test);
                if(is_identical(rc4,rc4_test,256)) {
                    iv_key[4] = '\0';
                    printf("%s\n",iv_key+1);
                    return 0;
                }
            }
        }
    }
}

void swap(unsigned char* S, int i, int j) {
    unsigned char tmp = S[i];
    S[i] = S[j];
    S[j] = tmp;
}

void RC4(unsigned char* key, unsigned int keylength, unsigned char* S) {
    int i, j;
    for(i=0; i<256; i++) {
        S[i] = i;
    }
    j=0;
    for(i=0;i<256;i++) {
        j = (j+S[i]+key[i%keylength])%256;
        swap(S,i,j);
    }
}

uint32_t crc32_for_byte(uint32_t r) {
    int j;
    for(j=0; j<8; j++) {
        r = (r&1 ? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
    }
    return r^(uint32_t)0xFF000000L;
}

void crc32(const void* data, size_t n_bytes, uint32_t* crc) {
    static uint32_t table[0x100];
    size_t i;
    if(!*table) {
        for(i=0; i<0x100; i++) {
            table[i] = crc32_for_byte(i);
        }
    }
    for(i=0; i<n_bytes; i++) {
        *crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc>>8;
    }
}

char is_identical(unsigned char* a, unsigned char* b, unsigned int n) {
    int i;
    for(i=0;i<n;i++) {
        if(a[i]!=b[i])
            return 0;
    }
    return 1;
}
