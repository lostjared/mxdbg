#include<stdio.h>

extern void echo_text(const char *src);

int main() {
    echo_text("Hello, World!");
    return 0;
}