#include<stdio.h>
#include<stdlib.h>
#include"intstack.h"


int main() {
    init_stack();
    int active = 1;
    while(active == 1) {
        printf("1 for push, 2 for pop, 3 for list: ");
        char buffer[256] = {0};
        fgets(buffer, sizeof(buffer), stdin);
        int choice = atoi(buffer);
        switch(choice) {
            case 1: {
                printf("Enter value to push: ");
                long value = 0;
                char buf[256] = {0};
                fgets(buf, sizeof(buf), stdin);
                value = atol(buf);
                push_stack(value);
                printf("pushed: %ld\n", value);
            }
            break;
            case 2:
                pop_stack();
            break;
            case 3:
                print_stack();
            break;
            default:
            printf("Error input...\n");
        }
    }
    return 0;
}