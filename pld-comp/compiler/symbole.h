#ifndef SYMBOLE_H
#define SYMBOLE_H

#include <string>
#include "type.h"

// Structure représentant un symbole (variable, fonction, etc.)
// Cette version minimale stocke le nom, le type et l'offset (pour la gestion de la mémoire, par exemple sur la pile).
struct Symbole {
    std::string name;  // Nom du symbole (par exemple, le nom d'une variable)
    Type type;         // Le type du symbole (INT, CHAR, VOID, etc.)
    int offset;        // Offset sur la pile ou dans la zone d'allocation (en octets)
};

#endif // SYMBOLE_H
