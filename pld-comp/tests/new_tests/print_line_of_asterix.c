#include <stdio.h>

void print_line(int n) {
    while (n > 0) {
        putchar('*');
        n = n - 1;
    }
}

int main() {
    int count = 10;
    print_line(count);
    putchar(10);
    return 0;
}
