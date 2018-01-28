#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_LETTERS 26

char cshift(char c, int n_shift, char base);

int main(int argc, char** argv) {
    if(argc != 3) {
        printf("Usage: ./caesar_decrypt <\"message\"> <num_shift>\n");
        exit(1);
    }

    char* word = argv[1];
    int num_shift = atoi(argv[2]);

    if(num_shift < 0) {
        printf("<num_shift> must be positive.\n");
        exit(1);
    }

    unsigned int word_len = strlen(word);
    char c;
    for(unsigned int i=0; i<word_len; i++) {
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
