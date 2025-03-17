#include "SymbolTableVisitor.h"

antlrcpp::Any
SymbolTableVisitor::visitAssignment(ifccParser::AssignmentContext *ctx)
{
  std::string varName = ctx->ID()->getText();

  // Check if variable is already declared
  if (symbolTable.find(varName) != symbolTable.end())
  {
    std::cerr << "Error: Variable '" << varName
              << "' is declared multiple times.\n";
    exit(1);
  }

  // Assign an offset for the new variable
  symbolTable[varName] = currentOffset;
  currentOffset -= 4; // Move stack pointer down for the next variable

  // std::cout << "[DEBUG] Assigned '" << varName << "' to offset "
  //           << symbolTable[varName] << "\n";

  // Visit the right-hand side of the assignment
  visit(ctx->expr());

  return 0;
}

antlrcpp::Any
SymbolTableVisitor::visitOperand(ifccParser::OperandContext *ctx)
{
  if (ctx->ID())
  {
    std::string varName = ctx->ID()->getText();

    // Check if variable was declared before being used
    if (symbolTable.find(varName) == symbolTable.end())
    {
      std::cerr << "Error: Variable '" << varName
                << "' is used before being declared.\n";
      exit(1);
    }

    // Mark the variable as used
    usedVariables.insert(varName);
  }
  return 0;
}

void SymbolTableVisitor::checkUnusedVariables()
{
  for (const auto &entry : symbolTable)
  {
    if (usedVariables.find(entry.first) == usedVariables.end())
    {
      std::cerr << "Warning: Variable '" << entry.first
                << "' is declared but never used.\n";
    }
  }
}
