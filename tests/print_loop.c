#include<stdio.h>

extern void echo_text(const char *src);
extern void echo_backwards(const char *src);

int main() {
    echo_text("Hello, World!");
    echo_backwards("Hello, World!");
    return 0;
}