#include <stdio.h>

int multiply(int a, int b) {
    int result = 0;
    int i = 0;
    while (i < b) {
        result = result + a;
        i = i + 1;
    }
    return result;
}

int main() {
    int a = 4;
    int b = 3;
    int product;
    product = multiply(a, b);
    return prduct;
}
