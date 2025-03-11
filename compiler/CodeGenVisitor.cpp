#include "CodeGenVisitor.h"

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
  std::cout << ".globl main\n";
  std::cout << " main: \n";
  std::cout << "    pushq %rbp \n";
  std::cout << "    movq %rsp, %rbp \n";

  // Visit assignments
  for (auto assignment : ctx->assignment()) {
    visit(assignment);
  }

  this->visit(ctx->return_stmt());

  std::cout << "    popq %rbp\n";
  std::cout << "    ret\n";

  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
  visit(ctx->operand());
  // int retval = stoi(ctx->CONST()->getText());

  // std::cout << "    movl $" << retval << ", %eax\n";

  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitAssignment(ifccParser::AssignmentContext *ctx) {
  std::string varName = ctx->ID()->getText();

  if (symbolTable.find(varName) == symbolTable.end()) {
    std::cerr << "Error: Undefined variable '" << varName
              << "' during code generation.\n";
    exit(1);
  }

  // Visit the operand (evaluate right-hand side expression)
  visit(ctx->operand());

  // Store the result in the variable’s stack location
  std::cout << "    movl %eax, " << symbolTable[varName] << "(%rbp)\n";

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitOperand(ifccParser::OperandContext *ctx) {
  if (ctx->CONST()) {
    // Load constant into eax
    int value = std::stoi(ctx->CONST()->getText());
    std::cout << "    movl $" << value << ", %eax\n";
  } else if (ctx->ID()) {
    std::string varName = ctx->ID()->getText();

    if (symbolTable.find(varName) == symbolTable.end()) {
      std::cerr << "Error: Undefined variable '" << varName
                << "' during code generation.\n";
      exit(1);
    }

    // Load variable value from memory into eax
    std::cout << "    movl " << symbolTable[varName] << "(%rbp), %eax\n";
  }
  return 0;
}