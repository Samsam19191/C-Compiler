#pragma once


#include "antlr4-runtime.h"
#include <unordered_map>
#include <iostream>
#include "generated/ifccBaseVisitor.h"


class  CodeGenVisitor : public ifccBaseVisitor {
private:
  std::unordered_map<std::string, int> symbolTable; // Stores variable offsets
  std::set<std::string> initializedVariables; // Stores initialized variables

public:
  void setSymbolTable(const std::unordered_map<std::string, int> &table) {
    symbolTable = table; // Copy symbol table from SymbolTableVisitor
  }
  void setInitializedVariables(const std::set<std::string> &vars) {
    initializedVariables = vars; // Copy initialized variables from SymbolTableVisitor
  }

  virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
  antlrcpp::Any visitStatement(ifccParser::StatementContext *ctx) override;
  antlrcpp::Any visitAssignment(ifccParser::AssignmentContext *ctx) override;
  antlrcpp::Any visitDeclaration(ifccParser::DeclarationContext *ctx) override;
  antlrcpp::Any visitOperand(ifccParser::OperandContext *ctx) override;
  antlrcpp::Any visitOperandExpr(ifccParser::OperandExprContext *ctx) override;
  antlrcpp::Any visitMulDiv(ifccParser::MulDivContext *ctx) override;
  antlrcpp::Any visitAddSub(ifccParser::AddSubContext *ctx) override;
  antlrcpp::Any visitParens(ifccParser::ParensContext *ctx) override;

  virtual antlrcpp::Any
  visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;
};

