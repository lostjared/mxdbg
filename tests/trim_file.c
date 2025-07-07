#include<stdio.h>
#include<stdlib.h>

extern void trim(FILE *fptr, const char *src);

int main(int argc, char **argv) {
    if(argc != 3) {
        printf("Error requires two arguments...\n");
        return EXIT_FAILURE;
    }
    FILE *fptr = fopen(argv[1], "r");
    if(!fptr) {
        fprintf(stderr, "Error could not open %s", argv[1]);
        return EXIT_FAILURE;
    }
    FILE *fout = fopen(argv[2], "w");
    if(!fout) {
        fprintf(stderr, "Error opening file: %s\n", argv[2]);
        fclose(fptr);
        return EXIT_FAILURE;
    }
    unsigned int i = 0;
    while(!feof(fptr)) {
        char buffer[256] = {0};
        fgets(buffer, 255, fptr);
        printf("%d: %s\n", i++, buffer);
        trim(fout, buffer);
    }
    fclose(fptr);
    fclose(fout);
    return EXIT_SUCCESS;
}