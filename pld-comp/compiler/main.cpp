#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include "CodeGenVisitor.h"
#include "SymbolTableVisitor.h"
#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"
#include "IR.h" // Pour CFG et DefFonction

using namespace antlr4;
using namespace std;

int main(int argn, const char **argv)
{
  stringstream in;
  if (argn == 2)
  {
    ifstream lecture(argv[1]);
    if (!lecture.good())
    {
      cerr << "error: cannot read file: " << argv[1] << endl;
      exit(1);
    }
    in << lecture.rdbuf();
  }
  else
  {
    cerr << "usage: ifcc path/to/file.c" << endl;
    exit(1);
  }

  ANTLRInputStream input(in.str());
  ifccLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  tokens.fill();

  ifccParser parser(&tokens);
  tree::ParseTree *tree = parser.axiom();

  if (parser.getNumberOfSyntaxErrors() != 0)
  {
    cerr << "error: syntax error during parsing" << endl;
    exit(1);
  }

  SymbolTableVisitor stv;
  stv.visit(tree);
  stv.checkUnusedVariables();

  DefFonction *defMain = new DefFonction("main");
  CFG *cfg = new CFG(defMain);
  cfg->setSymbolTable(stv.getSymbolTable());

  // Définir la cible par défaut à x86
  cfg->target = TargetArch::X86;

  CodeGenVisitor v;
  v.setSymbolTable(stv.getSymbolTable());
  v.setInitializedVariables(stv.getInitializedVariables());
  v.setCFG(cfg);
  v.visit(tree);

  cfg->gen_asm(std::cout);

  return 0;
}
