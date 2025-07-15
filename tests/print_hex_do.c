#include<stdio.h>
#include<stdlib.h>

void print_hex_stream(FILE *fptr, FILE *fout) {
    size_t bytes_read = 0;
    do {
        char buffer[15] = {0};
        bytes_read = fread(buffer, 1, 15, fptr);
        if(bytes_read == 0)
            break;
        for(size_t i = 0; i < bytes_read; ++i) {
            fprintf(fout, "%02x ", (unsigned char)buffer[i]);
        }
        for(size_t i = bytes_read; i < 16; ++i) {
            fprintf(fout, "   ");
        }
        fprintf(fout, " |");
        for(size_t i = 0; i < bytes_read; ++i) {
            if(buffer[i] < 32 || buffer[i] > 126) {
                fprintf(fout, ".");
            } else {
                fprintf(fout, "%c", buffer[i]);
            }
        }
        fprintf(fout, "|\n");
    } while (bytes_read != 0);
}

int main(int argc, char **argv) {
    if(argc != 3) {
        fprintf(stderr, "Error requires two arguments.\n");
        fprintf(stderr, "%s: <input> <output>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    FILE *fptr = fopen(argv[1], "rb");
    if(!fptr) {
        fprintf(stderr, "Error opening file: %s", argv[1]);
        exit(EXIT_FAILURE);
    }
    FILE *fout = fopen(argv[2], "w");
    if(!fout) {
        fprintf(stderr, "Error opening output file: %s\n", argv[2]);
        fclose(fptr);
        exit(EXIT_FAILURE);
    }
    print_hex_stream(fptr, fout);
    fclose(fptr);
    fclose(fout);
    return 0;
}