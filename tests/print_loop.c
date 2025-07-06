#include<stdio.h>

extern void echo_text(const char *src);
extern void echo_backwards(const char *src);
extern void echo_for(const char *src);

int main() {
    echo_text("Hello, World!");
    echo_backwards("Hello, World!");
    echo_for("Hey world");
    return 0;
}