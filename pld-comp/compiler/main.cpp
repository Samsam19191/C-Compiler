#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include "CodeGenVisitorV2.h"
#include "SymbolTableVisitor.h"
#include "IR.h"  // Pour CFG, etc.
#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"

using namespace antlr4;
using namespace std;

int main(int argn, const char **argv) {
  stringstream in;
  if (argn == 2) {
    ifstream lecture(argv[1]);
    if (!lecture.good()) {
      cerr << "error: cannot read file: " << argv[1] << endl;
      exit(1);
    }
    in << lecture.rdbuf();
  } else {
    cerr << "usage: ifcc path/to/file.c" << endl;
    exit(1);
  }

  ANTLRInputStream input(in.str());

  ifccLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  tokens.fill();

  ifccParser parser(&tokens);
  tree::ParseTree *tree = parser.axiom();

  if (parser.getNumberOfSyntaxErrors() != 0) {
    cerr << "error: syntax error during parsing" << endl;
    exit(1);
  }

  // Visiteur de la table des symboles
  SymbolTableVisitor stv;
  stv.visit(tree);
  stv.checkUnusedVariables(); // Check for unused variables

  // Création du CFG pour construire l'IR.
  // Ici, nous passons nullptr comme DefFonction* car nous n'avons pas encore de structure dédiée.
  CFG* cfg = new CFG(nullptr);
  BasicBlock* bb0 = new BasicBlock(cfg, cfg->new_BB_name());
  cfg->add_bb(bb0);


  // Visiteur de génération de code
  CodeGenVisitorV2 v;
  v.setSymbolTable(stv.getSymbolTable()); // Transfert de la table des symboles
  v.setCFG(cfg);  // On passe le CFG pour que CodeGenVisitor y ajoute des instructions IR
  v.visit(tree);

  // Une fois l'IR construite, on génère le code assembleur final.
  cfg->gen_asm(std::cout);

  // (Optionnel) Nettoyage de la mémoire allouée pour le CFG
  // delete cfg; 

  return 0;
}