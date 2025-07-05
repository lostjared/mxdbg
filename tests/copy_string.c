#include<stdio.h>

extern void string_cat(char *dst, const char *src);

int main() {
    char buffer[256] = {0};
    string_cat(buffer , "hey ");
    string_cat(buffer, " from asm\n");
    printf("%s\n", buffer);
    return 0;
}