#include<stdio.h>
#include<stdlib.h>

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "Error requires one argument.\n");
        exit(EXIT_FAILURE);
    }
    FILE *fptr = fopen(argv[1], "rb");
    if(!fptr) {
        fprintf(stderr, "Error opening file: %s", argv[1]);
        exit(EXIT_FAILURE);
    }
    while(1) {
        char buffer[15] = {0};
        size_t bytes_read = fread(buffer, 1, 15, fptr);
        if(bytes_read == 0)
            break;
        for(size_t i = 0; i < bytes_read; ++i) {
            fprintf(stdout, "%02x ", (unsigned char)buffer[i]);
        }
        for(size_t i = bytes_read; i < 16; ++i) {
            fprintf(stdout, "   ");
        }
        fprintf(stdout , " |");
        for(size_t i = 0; i < bytes_read; ++i) {
            if(buffer[i] < 32 || buffer[i] > 126) {
                fprintf(stdout, ".");
            } else {
                fprintf(stdout, "%c", buffer[i]);
            }
        }
        fprintf(stdout, "|\n");
    }
    fclose(fptr);
    return 0;
}