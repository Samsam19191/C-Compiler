#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

// Inclusion des fichiers nécessaires pour ANTLR et le générateur de code
#include "antlr4-runtime.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"

#include "CodeGenVisitor.h"

using namespace antlr4;
using namespace std;

int main(int argn, const char **argv) {
  stringstream in;

  // Vérifie si un fichier a été passé en argument
  if (argn == 2) {
    ifstream lecture(argv[1]); // Ouvre le fichier en lecture
    if (!lecture.good()) { // Vérifie si le fichier est lisible
      cerr << "error: cannot read file: " << argv[1] << endl;
      exit(1); // Quitte le programme en cas d'erreur
    }
    in << lecture.rdbuf(); // Charge le contenu du fichier dans un flux
  } else {
    // Affiche un message d'utilisation si aucun fichier n'est fourni
    cerr << "usage: ifcc path/to/file.c" << endl;
    exit(1);
  }

  // Crée un flux d'entrée pour ANTLR à partir du contenu du fichier
  ANTLRInputStream input(in.str());

  // Initialise le lexer pour analyser les tokens
  ifccLexer lexer(&input);
  CommonTokenStream tokens(&lexer);

  tokens.fill(); // Remplit le flux de tokens

  // Initialise le parser pour analyser la grammaire
  ifccParser parser(&tokens);
  tree::ParseTree *tree = parser.axiom(); // Analyse l'arbre syntaxique

  // Vérifie s'il y a des erreurs de syntaxe
  if (lexer.getNumberOfSyntaxErrors() != 0 ||
      parser.getNumberOfSyntaxErrors() != 0) {
    cerr << "error: syntax error during parsing" << endl;
    exit(1); // Quitte le programme en cas d'erreur
  }

  // Crée un visiteur pour générer le code
  CodeGenVisitor v;
  v.visit(tree); // Visite l'arbre syntaxique

  // Récupère la liste des CFG (Control Flow Graphs) générés
  auto cfgList = v.getCfgList();
  for (auto cfg : cfgList) {
    // Ignore les fonctions spéciales "putchar" et "getchar"
    if (cfg->get_name() == "putchar" || cfg->get_name() == "getchar") {
      continue;
    }
    // Génère le code assembleur pour chaque CFG
    cfg->gen_asm(cout);

    // Affiche le nom de la fonction sur la sortie d'erreur standard
    cerr << cfg->get_name() << endl;

    // Affiche les instructions de chaque bloc du CFG
    for (auto block : cfg->getBlocks()) {
      for (auto instr : block->instrs) {
        cerr << instr << endl;
      }
    }
  }

  return 0; // Fin du programme
}