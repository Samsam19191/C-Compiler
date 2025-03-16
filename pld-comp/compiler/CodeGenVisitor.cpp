#include "CodeGenVisitor.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext* ctx) {
  std::cout << ".globl main\n";
  std::cout << " main:\n";
  std::cout << "    pushq %rbp\n";
  std::cout << "    movq %rsp, %rbp\n";

  for (auto assignment : ctx->assignment()) {
    visit(assignment);
  }

  this->visit(ctx->return_stmt());

  std::cout << "    popq %rbp\n";
  std::cout << "    ret\n";

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext* ctx) {
  visit(ctx->expr());
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitAssignment(ifccParser::AssignmentContext* ctx) {
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

antlrcpp::Any CodeGenVisitor::visitOperand(ifccParser::OperandContext* ctx) {
    if (ctx->CONST()) {
        int value = std::stoi(ctx->CONST()->getText());
        std::cout << "    movl $" << value << ", %eax\n";
    }
    else if (ctx->CHAR()) {
        std::string literal = ctx->CHAR()->getText();
        char c = literal[1];
        int value = static_cast<int>(c);
        std::cout << "    movl $" << value << ", %eax\n";
    }
    else if (ctx->ID()) {
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

antlrcpp::Any CodeGenVisitor::visitMulDiv(ifccParser::MulDivContext* ctx) {
  visit(ctx->expr(1));
  std::cout << "    pushq %rax\n";

  visit(ctx->expr(0));
  std::cout << "    popq %rcx\n";

  if (ctx->getText().find("*") != std::string::npos) {
    std::cout << "    imul %rcx, %rax\n";
  } else {
    std::cout << "    cqo\n"; // Extension de %rax pour la division signée.
    std::cout << "    idiv %rcx\n";
  }

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitAddSub(ifccParser::AddSubContext* ctx) {
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

antlrcpp::Any CodeGenVisitor::visitParens(ifccParser::ParensContext* ctx) {
  return visit(ctx->expr());
}

antlrcpp::Any CodeGenVisitor::visitOperandExpr(ifccParser::OperandExprContext* ctx) {
  return visit(ctx->operand());
}

antlrcpp::Any CodeGenVisitor::visitFuncCall(ifccParser::FuncCallContext *ctx) {
    std::string functionName = ctx->ID()->getText();

    int argIndex = 0;
    std::vector<std::string> registers = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};

    for (auto expr : ctx->expr()) {
        visit(expr);

        if (argIndex < registers.size()) {
            std::cout << "    movq %rax, " << registers[argIndex] << "\n";
        } else {
            std::cerr << "Error: Too many arguments for function '" << functionName << "'.\n";
            exit(1);
        }
        argIndex++;
    }

    std::cout << "    call " << functionName << "\n";

    return 0;
}

