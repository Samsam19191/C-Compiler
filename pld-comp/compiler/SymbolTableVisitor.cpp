#include "SymbolTableVisitor.h"

// antlrcpp::Any
// SymbolTableVisitor::visitAssignment(ifccParser::AssignmentContext *ctx) {

//   std::vector<antlr4::tree::TerminalNode *> varNames = ctx->ID();
//   std::vector<ifccParser::ExprContext *> exprs = ctx->expr();

//   for (int i = 0; i < varNames.size(); i++) {
//     std::string varName = varNames[i]->getText();
//     ifccParser::ExprContext *expr = exprs[i];

//     if (symbolTable.find(varName) != symbolTable.end()) {
//       std::cerr << "Error: Variable '" << varName
//                 << "' is declared multiple times.\n";
//       exit(1);
//     }

//     symbolTable[varName] = currentOffset;
//     currentOffset -= 4;

//     visit(expr);
//   }

//   return 0;
// }

antlrcpp::Any
SymbolTableVisitor::visitDeclaration(ifccParser::DeclarationContext *ctx) {
  std::string type = ctx->type()->getText();
  std::vector<antlr4::tree::TerminalNode *> varNames = ctx->ID();
  std::vector<ifccParser::ExprContext *> exprs = ctx->expr();
  int assignments = exprs.size();
  for (int i = 0; i < varNames.size(); i++) {
    antlr4::tree::TerminalNode *varName = varNames[i];

    if (symbolTable.find(varName->getText()) != symbolTable.end()) {
      std::cerr << "Error: Variable '" << varName->getText()
                << "' is declared multiple times.\n";
      exit(1);
    }

    // offset by 1 if char
    if (type == "char") {
      currentOffset -= 1;
    } else {
      currentOffset -= 4;
    }
    symbolTable[varName->getText()] = currentOffset;

    // only visit the right-hand side of the expression if it exists
    if (assignments) {
      visit(exprs[i]);
      initializedVariables.insert(varName->getText());
    }
  }

  return 0;
}

antlrcpp::Any
SymbolTableVisitor::visitOperandExpr(ifccParser::OperandExprContext *ctx) {
  // Visit the right-hand side of the expression
  visit(ctx->operand());
  return 0;
}

antlrcpp::Any
SymbolTableVisitor::visitOperand(ifccParser::OperandContext *ctx) {
  if (ctx->ID()) {
    std::string varName = ctx->ID()->getText();

    // Check if variable was declared before being used
    if (symbolTable.find(varName) == symbolTable.end()) {
      std::cerr << "Error: Variable '" << varName
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
      std::cerr << "Warning: Variable '" << entry.first
                << "' is declared but never used.\n";
    }
  }
}
