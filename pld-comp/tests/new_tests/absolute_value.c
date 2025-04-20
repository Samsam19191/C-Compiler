#include <stdio.h>

int abs_val(int n) {
    if (n < 0) {
        return -n;
    } else {
        return n;
    }
}

int main() {
    int num = -123;
    int a;
    a = abs_val(num);
    return a;
}
