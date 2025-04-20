#include <stdio.h>

void countdown(int n) {
    print_int(n);
    putchar(' ');
    if (n > 0) {
        countdown(n - 1);
    }
}

int main() {
    countdown(5);
    putchar(10);
    return 0;
}
