#include <stdio.h>

int count_digits(int n) {
    if (n < 0) {
        n = -n;
    }
    if (n < 10) {
        return 1;
    } else {
        return 1 + count_digits(n / 10);
    }
}

int main() {
    int number = 12345;
    int result;
    result = count_digits(number);
    return result;
}
