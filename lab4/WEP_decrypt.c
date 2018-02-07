#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void RC4(unsigned char* key, unsigned int keylength, unsigned char* S);
void crc32(const void* data, size_t n_bytes, uint32_t* crc);

int main() {
    unsigned char* challenge = "hello";
    unsigned char* key = "ECE";
    unsigned int keylength = (unsigned int)strlen(key)+1;
    unsigned int checksum=0, test_checksum=0;
    unsigned char iv_key[keylength];
    int i;
    // key part of iv_key always the same
    for(i=1;i<keylength; i++){
        iv_key[i] = key[i-1];
    }
    unsigned char buff[257]; // input buffer
    unsigned char c[256]; // the message
    unsigned char S[256];

//    write(1, challenge, strlen(challenge));
    read(0, buff, 257);
    iv_key[0] = buff[0]; // first byte is the IV
    for(i=0;i<256;i++) {
        c[i] = buff[i+1];
    }
    RC4(iv_key, keylength, S);
    for(i=0;i<256;i++) {
        c[i]=c[i]^S[i]; // get the message back
    }
    printf("Received Message: %s\n",c);
    
    // the last 4 bytes of c is the CRC32
    checksum = c[252]<<24 | c[253]<<16 | c[254]<<8 | c[255];
    crc32(c, 252, &test_checksum);
    if(checksum == test_checksum) {
        printf("Connection Accepted.\n");
    }
    else {
        printf("Connection Denied.\n");
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
