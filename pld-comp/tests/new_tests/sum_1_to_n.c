#include <stdio.h>

int main() {
    int n = 5;
    int sum = 0;
    int i = 1;

    while (i <= n) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}
