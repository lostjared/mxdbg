#include<stdio.h>
#include<stdlib.h>
#include<string.h>

extern char **tokenize(const char *src, const char sep, int *count);

int main() {
     int count = 0;
     char buffer[256] = {0};
     printf("Enter string to tokenize/split: ");
     fgets(buffer, sizeof(buffer), stdin);
     size_t len = strlen(buffer);
     if(len > 0) {
        buffer[len-1] = 0;
     }
     char **arr = tokenize(buffer, ' ', &count);
     printf("%p: %d tokens\n", arr, count);
     for(int i = 0; i < count; ++i) {
         printf("Token: %d -> [%p][%s]\n", i, arr[i], arr[i]);
     }
     for(int i = 0; i < count; ++i) {
        printf("Freeing [%s]\n", arr[i]);
        free(arr[i]);
        arr[i] = NULL;
     }
    free(arr);
    return 0;
}