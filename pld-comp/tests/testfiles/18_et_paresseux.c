int main() {
    int a = 0;
    int b = 0;

    // Le second incrément ne doit PAS être évalué si a est faux
    if (0 && (++a || ++b)) {
        // ne rentre pas ici
    }

    return a + b * 10; // doit retourner 0
}
