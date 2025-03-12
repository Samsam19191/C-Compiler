#include "CodeGenVisitor.h"

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
  std::cout << ".globl main\n";
  std::cout << " main: \n";
  std::cout << "    pushq %rbp \n";
  std::cout << "    movq %rsp, %rbp \n";

  // Visit assignments
  for (auto statement : ctx->statement()) {
    visit(statement);
  }

  this->visit(ctx->return_stmt());

  std::cout << "    popq %rbp\n";
  std::cout << "    ret\n";

  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitStatement(ifccParser::StatementContext *ctx) {
  if(ctx->declaration()) {
    return visit(ctx->declaration());
  } else if(ctx->assignment()) {
    return visit(ctx->assignment());
  }
  
  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
  // visit(ctx->operand());
  visit(ctx->expr());
  // int retval = stoi(ctx->CONSTINT()->getText());

  // std::cout << "    movl $" << retval << ", %eax\n"; 

  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitAssignment(ifccParser::AssignmentContext *ctx) {
  const std::vector<antlr4::tree::TerminalNode*>& varNames = ctx->ID();
  const std::vector<ifccParser::ExprContext*>& exprs = ctx->expr();
  
  for(int i = 0; i < varNames.size(); i++) {
    std::string varName = varNames[i]->getText();
    ifccParser::ExprContext* expr = exprs[i];

    if (symbolTable.find(varName) == symbolTable.end()) {
      std::cerr << "Error: Undefined variable '" << varName
                << "' during code generation.\n";
      exit(1);
    }

    visit(expr);
    std::cout << "    movl %eax, " << symbolTable[varName] << "(%rbp)\n";
  }

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitOperand(ifccParser::OperandContext *ctx) {
  if (ctx->CONSTINT()) {
    int value = std::stoi(ctx->CONSTINT()->getText());
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