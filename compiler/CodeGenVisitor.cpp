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
  // visit(ctx->operand());
  visit(ctx->expr());
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

  visit(ctx->expr());

  std::cout << "    movl %eax, " << symbolTable[varName] << "(%rbp)\n";

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitOperand(ifccParser::OperandContext *ctx) {
  if (ctx->CONST()) {
    int value = std::stoi(ctx->CONST()->getText());
    std::cout << "    movl $" << value << ", %eax\n";
  } else if (ctx->ID()) {
    std::string varName = ctx->ID()->getText();

    if (symbolTable.find(varName) == symbolTable.end()) {
      std::cerr << "Error: Undefined variable '" << varName
                << "' during code generation.\n";
      exit(1);
    }

    std::cout << "    movl " << symbolTable[varName] << "(%rbp), %eax\n";
  }
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitMulDiv(ifccParser::MulDivContext *ctx) {
  visit(ctx->expr(1));
  std::cout << "    pushq %rax\n";
  visit(ctx->expr(0));
  std::cout << "    popq %rcx\n";

  if (ctx->getText().find("*") != std::string::npos) {
    std::cout << "    imul %rcx, %rax\n";
  } else {
    std::cout << "    cqo\n"; // Convert %rax to 64-bit for division
    std::cout << "    idiv %rcx\n";
  }

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitAddSub(ifccParser::AddSubContext *ctx) {
  visit(ctx->expr(0));
  std::cout << "    pushq %rax\n";
  visit(ctx->expr(1));
  std::cout << "    popq %rcx\n";

  if (ctx->getText().find("+") != std::string::npos) {
    std::cout << "    add %rcx, %rax\n";
  } else {
    std::cout << "    sub %rcx, %rax\n";
  }

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitParens(ifccParser::ParensContext *ctx) {
  return visit(ctx->expr());
}

antlrcpp::Any
CodeGenVisitor::visitOperandExpr(ifccParser::OperandExprContext *ctx) {
  return visit(ctx->operand());
}

// antlrcpp::Any CodeGenVisitor::visitExpr(ifccParser::ExprContext *ctx) {
//   if (auto mulDivCtx = dynamic_cast<ifccParser::MulDivContext *>(ctx)) {
//     visit(mulDivCtx->expr(0));       // Left operand
//     std::cout << "    pushq %rax\n"; // Save left operand on stack
//     visit(mulDivCtx->expr(1));       // Right operand
//     std::cout << "    popq %rcx\n";  // Restore left operand into %rcx

//     if (mulDivCtx->getText().find("*") != std::string::npos) {
//       std::cout << "    imul %rcx, %rax\n";
//     } else {
//       std::cout << "    cqo\n";       // Convert %rax to 64-bit for division
//       std::cout << "    idiv %rcx\n"; // Signed division: %rax = %rax / %rcx
//     }
//   } else if (auto addSubCtx = dynamic_cast<ifccParser::AddSubContext *>(ctx))
//   {
//     visit(addSubCtx->expr(0));       // Left operand
//     std::cout << "    pushq %rax\n"; // Save left operand on stack
//     visit(addSubCtx->expr(1));       // Right operand
//     std::cout << "    popq %rcx\n";  // Restore left operand into %rcx

//     if (addSubCtx->getText().find("+") != std::string::npos) {
//       std::cout << "    add %rcx, %rax\n";
//     } else {
//       std::cout << "    sub %rcx, %rax\n";
//     }
//   } else if (auto parensCtx = dynamic_cast<ifccParser::ParensContext *>(ctx))
//   {
//     visit(parensCtx->expr()); // Evaluate inside parentheses
//   } else if (auto operandCtx =
//                  dynamic_cast<ifccParser::OperandExprContext *>(ctx)) {
//     visit(operandCtx->operand());
//   }

//   return 0;
// }
