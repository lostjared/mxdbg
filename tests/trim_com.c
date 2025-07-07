#include<stdio.h>
#include<stdlib.h>

extern void trim(const char *src);

int main(int argc, char **argv) {
    if(argc != 2) {
        printf("Error requires one argument...\n");
        return EXIT_FAILURE;
    }
    FILE *fptr = fopen(argv[1], "r");
    if(!fptr) {
        fprintf(stderr, "Error could not open data.txt");
        return EXIT_FAILURE;
    }
    while(!feof(fptr)) {
        char buffer[256] = {0};
        fgets(buffer, 255, fptr);
        trim(buffer);
    }
    fclose(fptr);
    return EXIT_SUCCESS;
}