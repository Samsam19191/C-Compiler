#include <stdio.h>

void print_int(int x) {
    if (x < 0) { putchar('-'); x = -x; }
    if (x / 10) { print_int(x / 10); }
    putchar((x % 10) + '0');
}

int main() {
    int n = 10;
    int sum = 0;
    int i = 0;
    while (i <= n) {
        if (i % 2 == 0) {
            sum = sum + i;
        }
        i = i + 1;
    }
    return sum;
}
