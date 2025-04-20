#include <stdio.h>

int main() {
    int n = 5;
    int result = 1;

    while (n > 1) {
        result = result * n;
        n = n - 1;
    }
    putchar(10);
    return result;
}
