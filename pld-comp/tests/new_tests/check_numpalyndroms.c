#include <stdio.h>

void print_int(int x) {
    if (x < 0) {
        putchar('-');
        x = -x;
    }
    if (x / 10) {
        print_int(x / 10);
    }
    putchar((x % 10) + '0');
}

int main() {
    int orig = 1221;
    int num = orig;
    int rev = 0;

    while (num != 0) {
        rev = rev * 10 + (num % 10);
        num = num / 10;
    }

    if (rev == orig) {
        putchar('Y');
    } else {
        putchar('N');
    }
    putchar(10);
    return 0;
}
