#include "SymbolTableVisitor.h"

using namespace std;

// Visite un nœud d'assignation dans l'AST
antlrcpp::Any
SymbolTableVisitor::visitAssignment(ifccParser::AssignmentContext *ctx) {
  // Récupère le nom de la variable assignée
  string variableName = ctx->ID()->getText();

  // Vérifie si la variable a déjà été déclarée
  if (symbolTable.find(variableName) != symbolTable.end()) {
    cerr << "Error: Variable '" << variableName
              << "' is declared multiple times.\n";
    exit(1); // Arrête le programme en cas d'erreur
  }

  // Assigne un offset pour la nouvelle variable
  symbolTable[variableName] = currentOffset;
  currentOffset -= 4; // Réduit l'offset pour la prochaine variable
  visit(ctx->expr()); // Visite l'expression associée à l'assignation

  return 0;
}

// Visite un nœud opérande dans l'AST
antlrcpp::Any
SymbolTableVisitor::visitOperand(ifccParser::OperandContext *ctx) {
  if (ctx->ID()) { // Si l'opérande est une variable
    string variableName = ctx->ID()->getText();

    // Vérifie si la variable a été déclarée avant utilisation
    if (symbolTable.find(variableName) == symbolTable.end()) {
      cerr << "Error: Variable '" << variableName
                << "' is used before being declared.\n";
      exit(1); // Arrête le programme en cas d'erreur
    }

    // Marque la variable comme utilisée
    usedVariables.insert(variableName);
  }
  return 0;
}

// Vérifie les variables déclarées mais jamais utilisées
void SymbolTableVisitor::checkUnusedVariables() {
  for (const auto &entry : symbolTable) {
    if (usedVariables.find(entry.first) == usedVariables.end()) {
      cerr << "Warning: Variable '" << entry.first
                << "' is declared but never used.\n";
    }
  }
}

// Visite un nœud d'appel de fonction dans l'AST
antlrcpp::Any SymbolTableVisitor::visitFuncCall(ifccParser::FuncCallContext *ctx) {
    // Récupère le nom de la fonction appelée
    string functionName = ctx->ID()->getText();

    // Vérifie si la fonction est définie (seules "putchar" et "getchar" sont autorisées)
    if (functionName != "putchar" && functionName != "getchar") {
        cerr << "Error: Undefined function '" << functionName << "'.\n";
        exit(1); // Arrête le programme en cas d'erreur
    }

    // Visite les expressions passées en arguments à la fonction
    for (auto expr : ctx->expr()) {
        visit(expr);
    }

    return 0;
}

// Visite un nœud de déclaration de variable dans l'AST
antlrcpp::Any SymbolTableVisitor::visitVar_decl_stmt(ifccParser::Var_decl_stmtContext *ctx) {
  for (auto member : ctx->var_decl_member()) {
    // Récupère le nom de la variable déclarée
    string variableName = member->ID()->getText();

    // Vérifie si la variable est initialisée
    if (!member->expr()) {
      cerr << "Warning: Variable '" << variableName << "' is not initialized.\n";
    }
  }
  return 0;
}