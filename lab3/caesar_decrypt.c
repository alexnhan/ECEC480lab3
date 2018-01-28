#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_LETTERS 26

char cshift(char c, int n_shift, char base);

int main(int argc, char** argv) {
    if(argc != 2) {
        printf("Usage: ./caesar_decrypt <num_shift>\n");
        exit(1);
    }

    char word[128];
    read(0, word, 128);
    int num_shift = atoi(argv[1]);

    if(num_shift < 0) {
        printf("<num_shift> must be positive.\n");
        exit(1);
    }

    unsigned int word_len = strlen(word);
    char c;
    unsigned int i;
    for(i=0; i<word_len; i++) {
        c = word[i];
        if(c >= 'A' && c <= 'Z') {
            word[i] = cshift(c, num_shift, 'A');
        }
        else if(c >= 'a' && c <= 'z') {
            word[i] = cshift(c, num_shift, 'a');
        }
    }
    printf("%s\n", word);

    return 0;
}

char cshift(char c, int n_shift, char base) {
    return (((c-base)-n_shift)+NUM_LETTERS)%NUM_LETTERS + base;
}
