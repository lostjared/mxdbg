#include<stdio.h>
#include<stdlib.h>
#include<string.h>

extern char **tokenize(const char *src, const char sep, int *count);

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "Requires one argument..\n%s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    FILE *fptr = fopen(argv[1], "r");
    if (fptr == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fseek(fptr, 0, SEEK_END);
    long length = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    char *buffer = (char*) malloc ( (sizeof(char) * length) + 1);
    fread(buffer, length, 1, fptr);
    buffer[length] = 0;
    fclose(fptr);
    int count = 0;
    char **arr = tokenize(buffer,' ',&count);
    if(arr != NULL) {
        for(int i = 0; i < count; ++i) {
            if(arr[i] != NULL) {
                size_t len = strlen(arr[i]);
                if(len > 0) {
                    if(arr[i][len-1] == '\n') arr[i][len-1] = 0;
                    printf("%d: [%p][%s]\n", i, arr[i], arr[i]);
                }
                free(arr[i]);
            }
        }
        free(arr);
    }
    return EXIT_SUCCESS;
}