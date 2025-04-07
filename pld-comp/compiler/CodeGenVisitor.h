#pragma once

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "IR.h" // Inclusion de notre IR
#include <iostream>
#include <unordered_map>
#include <set>

class CodeGenVisitor : public ifccBaseVisitor
{
private:
  std::unordered_map<std::string, int> symbolTable; // Table des symboles
  std::set<std::string> initializedVariables;       // Variables initialisées
  CFG *cfg;                                         // Pointeur vers le CFG utilisé pour générer l'IR

public:
  void setSymbolTable(const std::unordered_map<std::string, int> &table)
  {
    symbolTable = table;
  }
  void setInitializedVariables(const std::set<std::string> &vars)
  {
    initializedVariables = vars;
  }
  void setCFG(CFG *cfg_) { cfg = cfg_; }

  virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
  antlrcpp::Any visitStatement(ifccParser::StatementContext *ctx) override;
  antlrcpp::Any visitAssignment(ifccParser::AssignmentContext *ctx) override;
  antlrcpp::Any visitDeclaration(ifccParser::DeclarationContext *ctx) override;
  antlrcpp::Any visitOperand(ifccParser::OperandContext *ctx) override;
  antlrcpp::Any visitOperandExpr(ifccParser::OperandExprContext *ctx) override;
  antlrcpp::Any visitMulDiv(ifccParser::MulDivContext *ctx) override;
  antlrcpp::Any visitAddSub(ifccParser::AddSubContext *ctx) override;
  antlrcpp::Any visitParens(ifccParser::ParensContext *ctx) override;
  antlrcpp::Any visitBitOps(ifccParser::BitOpsContext *ctx) override;
  virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;
  antlrcpp::Any visitFuncCall(ifccParser::FuncCallContext *ctx) override;
};
