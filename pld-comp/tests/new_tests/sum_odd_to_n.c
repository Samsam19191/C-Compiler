#include <stdio.h>

int main() {
    int n = 10;
    int sum = 0;
    int i = 1;
    while (i <= n) {
        if (i % 2 != 0) {
            sum = sum + i;
        }
        i = i + 1;
    }
    return sum;
}
