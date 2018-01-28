#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "table.h"

static char lookup(char c, char base);

int main(int argc, char** argv) {
    char word[128];
    read(0,word,128);

    unsigned int word_len = strlen(word);
    char c;
    unsigned int i;
    for(i=0; i<word_len; i++) {
        c = word[i];
        if(c >= 'A' && c <= 'Z') {
            word[i] = lookup(c, 'A');
        }
        else if(c >= 'a' && c <= 'z') {
            word[i] = lookup(c, 'a');
        }
    }
    printf("%s\n", word);

    return 0;
}

char lookup(char c, char base) {
    return LUT[c-base];
}
