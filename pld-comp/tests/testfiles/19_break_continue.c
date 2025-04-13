int main() {
    int i = 0;
    while (1) {
        i++;
        if (i > 5) {
            break;  // Sort de la boucle quand i > 5
        }
        if (i % 2 == 0) {
            continue;  // Passe à l'itération suivante si pair
        }
        // Ce code ne s'exécute que pour les nombres impairs <= 5
    }
    return i;  // Doit retourner 6 (car i=6 quand break est exécuté)
}