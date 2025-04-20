#include <stdio.h>

int power(int base, int exponent) {
    if (exponent == 0) {
        return 1;
    } else {
        return base * power(base, exponent - 1);
    }
}

int main() {
    int result;
    result = power(2, 5);
    return result;
}
