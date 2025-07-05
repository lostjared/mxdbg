#include<stdio.h>

extern int string_cat(char *dst, const char *src, unsigned long size);

int main() {
    char buffer[256] = {0};
    if(string_cat(buffer , "hey ", 1) == 0) {
        puts("1: copied");
    } else {
        puts("1: didn't copy");
    }
    if(string_cat(buffer, " from asm\n", 25) == 0) {
        puts("2: copied ");
    } else {
        puts("2: didn't copy");
    }

    if(string_cat(buffer, " values from copy", sizeof(buffer)) == 0) {
        puts("3: copied");
    } else {
        puts("3: didn't copy");
    }
    printf("%s\n", buffer);
    return 0;
}