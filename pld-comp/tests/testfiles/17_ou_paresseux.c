int main() {
    int a = 0;
    int b = 0;

    // Le second incrément ne doit PAS être évalué si a est vrai
    if (1 || (++a && ++b)) {
        // a et b doivent rester 0 (car non évalués)
    }

    return a + b * 10; // doit retourner 0
}