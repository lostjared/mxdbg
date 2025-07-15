#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

int main(int argc, char **argv) {
    if(argc != 3) {
        printf("Error requires two arguments.\n %s <file> <delay>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    FILE *fptr = fopen(argv[1], "r");
    if(!fptr) {
        fprintf(stderr, "Error opening file: %s", argv[1]);
        exit(EXIT_FAILURE);
    }
    size_t bytes_read = 0;
    int timeout = atoi(argv[2]);
    if(timeout <= 0) {
        fprintf(stderr, "Error invalid timeout...\n");
        exit(EXIT_FAILURE);
    }
    do {
        char byte_value = 0;
        bytes_read = fread(&byte_value, 1, 1, fptr);
        if(bytes_read == 0)
            break;
        printf("%c", byte_value);
        fflush(stdout);
        usleep(timeout * 1000);
    } while ( bytes_read != 0 );
    fclose(fptr);
    return 0;
}