#include <stdio.h>

int sum_digits(int n) {
    if (n < 0) {
        n = -n;
    }
    if (n < 10) {
        return n;
    } else {
        return (n % 10) + sum_digits(n / 10);
    }
}

int main() {
    int result;
    result = sum_digits(1234);
    return result;
}
