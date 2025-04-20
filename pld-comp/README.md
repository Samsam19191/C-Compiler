# 🧠 IFCC – IF-C-Compiler

Compilateur pédagogique pour un sous-ensemble du langage C, développé dans le cadre du module PLD-COMP à l’INSA Lyon.

Hexanome 4211 : Riad Dalaoui, Antoine Aubut, Djalil Chikhi, Rayan Hanader, Sami Taider, Youssef Chaouki, Samuel Louvet


---

## ✅ Fonctionnalités

Le compilateur IFCC supporte :
- Un langage proche du C (sans préprocessing, avec `int` et `char`)
- Analyse lexicale, syntaxique, sémantique
- Génération de code assembleur x86 à partir d'un fichier `.c`
- Entrées/sorties via `getchar` / `putchar`
- Structures de contrôle : `if`, `else`, `while`
- Fonctions avec paramètres, portées, shadowing
- Instructions comme `return`, `+=`, `--`, etc.

> La grammaire acceptée est un sous-ensemble du C, sans préprocesseur (les directives comme `#include` sont ignorées).

---

## 🏗️ Architecture

Le projet repose sur une architecture en plusieurs passes :

1. Analyse lexicale et syntaxique via **ANTLR4**
2. Construction de l’AST
3. Vérification sémantique (types, variables, etc.)
4. Génération de code intermédiaire (IR)
5. Traduction IR → Assembleur (x86)

---

## 🗂️ Structure du projet

pld-comp/
├── compiler/         ← Code source du compilateur
│   ├── *.cpp / *.h   ← Visiteurs, IR, CFG, génération ASM
│   ├── ifcc.g4       ← Grammaire ANTLR4
│   ├── Makefile      ← Compilation
│   └── configs/      ← Configs OS (Ubuntu, Fedora, etc.)
├── tests/            ← Tests unitaires et d'intégration
│   ├── testfiles/    ← Fichiers de test standards
│   ├── new_tests/    ← Nouveaux tests
│   ├── ifcc-test.py  ← Script d'automatisation des tests
│   └── ifcc-wrapper.sh
├── include/, lib/, jar/   ← Dépendances ANTLR
├── docs/             ← Documentation utilisateur et développeur

## 🧪 Build et tests

### ⚙️ Compilation

Pour compiler le compilateur **IFCC**, exécutez les commandes suivantes :

```bash
cd pld-comp/compiler/
make clean
make
```

📁 Assurez-vous que le fichier configs/config.mk est correctement configuré pour votre système d’exploitation (Ubuntu, Fedora, etc.) et que le chemin vers ANTLR4 y est correctement renseigné.

### 🧪 Lancement des tests
Depuis le répertoire pld-comp/tests, vous pouvez lancer les tests unitaires et d’intégration avec :
```bash
cd ../tests
python3 ifcc-test.py [--verbose | --debug | --wrapper] testfiles/*.c
python3 ifcc-test.py [--verbose | --debug | --wrapper] new_tests/*.c
```

Conditions nécessaires :

- Python doit être installé et accessible dans le PATH (remplacez python3 si votre système utilise une autre commande).
- Les répertoires testfiles/ et new_tests/ doivent contenir uniquement des fichiers .c.
- Le binaire ifcc doit avoir été compilé avec succès et se trouver dans pld-comp/compiler/.
- L’option --wrapper (ou -w) permet de spécifier un script alternatif à ifcc-wrapper.sh.

### 💡 Exemple : tester un fichier spécifique
Pour compiler un fichier unique exemple.c, remplacez la fin de la commande par :
```bash
python3 ifcc-test.py testfiles/exemple.c
# ou
python3 ifcc-test.py new_tests/exemple.c
```

📌 Les commandes de test doivent être exécutées depuis le répertoire tests/, car les scripts ifcc-test.py et ifcc-wrapper.sh s’y trouvent.
Le fichier .c peut être situé n’importe où sur votre machine, tant que son chemin est correctement indiqué.


Ce projet est disponible sur GitHub : https://github.com/Samsam19191/C-Compiler