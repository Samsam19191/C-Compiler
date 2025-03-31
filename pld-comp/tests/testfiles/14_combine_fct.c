int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int main() {
    int s;
    int d;
    s = add(21, 9);       // s = 15
    d = subtract(s, 3);   // d = 15 - 3 = 12
    return d;
}
