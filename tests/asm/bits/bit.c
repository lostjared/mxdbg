#include<stdio.h>
#include<stdlib.h>

extern void bitset(long long *value, unsigned int bit);
extern void bitreset(long long *value, unsigned int bit);

void printb(long long value) {
    for (int i = 63; i >= 0; i--) {
        printf("%d", (int)((value >> i) & 1));
    }
}

long long get_value(const char *msg) {
        long long value;
        char buffer[256] = {0};
        printf("%s", msg);
        fgets(buffer, sizeof(buffer), stdin);
        value = atol(buffer);
        return value;
}

int main() {

    int active = 1;
    while(active) {
        long long value = get_value("Enter number: ");
        unsigned int index = (unsigned int) get_value("Enter bit index: ");
        long long choice = get_value("1 to set, 2 to clear, 3 to quit: ");
        printf("value: %x = ", (unsigned int)value);
        printb(value);
        printf("\n");
        if(choice == 1) 
            bitset(&value, index);
        else if(choice == 2)
            bitreset(&value, index);
        else 
            break;
        
        printf("value: %x = ", (unsigned int)value);
        printb(value);
        printf("\n");
    }

    return 0;
}