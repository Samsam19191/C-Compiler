README - PLD-Comp - Rendu intermédiaire

Avancement du projet

À ce stade, notre compilateur supporte les fonctionnalités suivantes :

Types et déclarations

Déclaration de variables de type entier (int) et caractère (char).

Initialisation directe des variables lors de leur déclaration.

Affectations

Affectation simple (variable = constante, variable = variable).

Affectations multiples (a = b, c = d).

Expressions supportées

Opérations arithmétiques : Addition (+), Soustraction (-), Multiplication (*), Division (/), Modulo (%).

Opérations bit à bit : ET (&), OU (|), XOR (^).

Gestion des parenthèses pour déterminer l'ordre des opérations.

Entrée et sortie

Utilisation des fonctions standard putchar() et getchar() pour effectuer des entrées-sorties.

Vérifications sémantiques

Vérification que les variables utilisées sont déclarées.

Vérification qu'une variable n'est pas déclarée plusieurs fois.

Vérification qu'une variable déclarée est effectivement utilisée.

Génération de code

Génération de code assembleur (x86) en respectant les conventions de l'ABI.

Tests

Mise en place d'une série de tests automatisés pour comparer notre compilateur avec GCC.

Utilisation du Makefile

make ou make all : compile le projet et génère l'exécutable ifcc.

make clean : nettoie tous les fichiers générés par la compilation.

make gui FILE=chemin/vers/votre/fichier.c : affiche l'arbre syntaxique du fichier spécifié dans une fenêtre graphique.

make exec FILE=chemin/vers/votre/fichier.c : compile et exécute directement un fichier source C avec le compilateur ifcc.

Important : configurez votre système en adaptant le fichier config.mk à votre environnement (exemples fournis : ubuntu.mk, DI.mk, fedora.mk).

Méthodologie de projet

Gestion Agile :

Nous suivons des cycles de développement courts (sprints), chacun produisant un compilateur fonctionnel mais limité à un sous-ensemble spécifique du langage C.

Utilisation régulière du TDD (développement dirigé par les tests) pour valider chaque fonctionnalité dès son implémentation.

Collaboration :

L'équipe a clairement réparti les rôles (analyse syntaxique, sémantique, génération de code, infrastructure de tests) tout en assurant que chacun maîtrise les bases du projet.

Nous travaillons tous sur des branches séparées et un membre de l'hexanome a était désigné pour traiter les pull requests sur le main.

Prochaines étapes

Ajout des structures de contrôle (if, while).

Renforcement des vérifications sémantiques.

Exploration d'une représentation intermédiaire (IR).

L'équipe est motivée et prête à continuer le développement.