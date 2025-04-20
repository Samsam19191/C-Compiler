#include <stdio.h>

int main() {
    int size = 5;
    int i = 0;
    int j;

    while (i < size) {
        j = 0;
        while (j < size) {
            putchar('*');
            j = j + 1;
        }
        putchar(10);
        i = i + 1;
    }
    return 0;
}
