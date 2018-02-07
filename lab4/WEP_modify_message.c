#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void crc32(const void* data, size_t n_bytes, uint32_t* crc);

int main() {
    unsigned char modify_msg[256] = "world";
    unsigned int modify_length = strlen(modify_msg);
    unsigned char challenge[256];
    unsigned char cf[257];
    unsigned int modify_checksum;
    int i;

    read(0,challenge,256); // don't need the challenge
    read(0,cf,257);
    
    for(i=modify_length; i<256; i++) {
        modify_msg[i] = '\0';
    }
    crc32(modify_msg,modify_length,&modify_checksum);
    modify_msg[252] = modify_checksum>>24 & 0x000000FF;
    modify_msg[253] = modify_checksum>>16 & 0x000000FF;
    modify_msg[254] = modify_checksum>>8 & 0x000000FF;
    modify_msg[255] = modify_checksum & 0x000000FF;

    for(i=0;i<256;i++) {
        cf[i+1] = cf[i+1] ^ modify_msg[i];
    }
    write(1,cf,257);
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

