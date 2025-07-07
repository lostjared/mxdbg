#include<stdio.h>

extern void trim(const char *src);

int main() {
    trim("Hello # worldd\n#test one\nThis is a test.\n");
    return 0;
}