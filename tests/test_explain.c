#include <stdio.h>

int add_numbers(int a, int b) {
    return a + b;
}

int multiply_numbers(int a, int b) {
    int result = 0;
    for (int i = 0; i < b; i++) {
        result += a;
    }
    return result;
}

int main() {
    int x = 5;
    int y = 3;
    
    int sum = add_numbers(x, y);
    int product = multiply_numbers(x, y);
    
    printf("Sum: %d\n", sum);
    printf("Product: %d\n", product);
    
    return 0;
}
