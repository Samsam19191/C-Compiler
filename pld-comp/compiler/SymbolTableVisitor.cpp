#include "SymbolTableVisitor.h"

using namespace std;

antlrcpp::Any
SymbolTableVisitor::visitAssignment(ifccParser::AssignmentContext *ctx) {
  string varName = ctx->ID()->getText();

  // Check if variable is already declared
  if (symbolTable.find(varName) != symbolTable.end()) {
    cerr << "Error: Variable '" << varName
              << "' is declared multiple times.\n";
    exit(1);
  }

  // Assign an offset for the new variable
  symbolTable[varName] = currentOffset;
  currentOffset -= 4; // Move stack pointer down for the next variable

  // cout << "[DEBUG] Assigned '" << varName << "' to offset "
  //           << symbolTable[varName] << "\n";

  // Visit the right-hand side of the assignment
  visit(ctx->expr());

  return 0;
}

antlrcpp::Any
SymbolTableVisitor::visitOperand(ifccParser::OperandContext *ctx) {
  if (ctx->ID()) {
    string varName = ctx->ID()->getText();

    // Check if variable was declared before being used
    if (symbolTable.find(varName) == symbolTable.end()) {
      cerr << "Error: Variable '" << varName
                << "' is used before being declared.\n";
      exit(1);
    }

    // Mark the variable as used
    usedVariables.insert(varName);
  }
  return 0;
}

void SymbolTableVisitor::checkUnusedVariables() {
  for (const auto &entry : symbolTable) {
    if (usedVariables.find(entry.first) == usedVariables.end()) {
      cerr << "Warning: Variable '" << entry.first
                << "' is declared but never used.\n";
    }
  }
}

antlrcpp::Any SymbolTableVisitor::visitFuncCall(ifccParser::FuncCallContext *ctx) {
    string functionName = ctx->ID()->getText();

    if (functionName != "putchar" && functionName != "getchar") {
        cerr << "Error: Undefined function '" << functionName << "'.\n";
        exit(1);
    }

    for (auto expr : ctx->expr()) {
        visit(expr);
    }

    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitVar_decl_stmt(ifccParser::Var_decl_stmtContext *ctx) {
  for (auto member : ctx->var_decl_member()) {
    string varName = member->ID()->getText();
    // ... (code existant)
    if (!member->expr()) {
      cerr << "Warning: Variable '" << varName << "' is not initialized.\n";
    }
  }
  return 0;
}