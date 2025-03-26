#pragma once

#include "antlr4-runtime.h"
#include <unordered_map>
#include <iostream>
#include "generated/ifccBaseVisitor.h"

class CodeGenVisitor : public ifccBaseVisitor {
class CodeGenVisitor : public ifccBaseVisitor {
private:
  std::unordered_map<std::string, int> symbolTable; // Stores variable offsets
  std::set<std::string> initializedVariables;       // Stores initialized variables

public:
  // Copie la table des symboles depuis le visiteur de la table des symboles.
  void setSymbolTable(const std::unordered_map<std::string, int>& table) {
    symbolTable = table;
  // Copie la table des symboles depuis le visiteur de la table des symboles.
  void setSymbolTable(const std::unordered_map<std::string, int>& table) {
    symbolTable = table;
  }
  void setInitializedVariables(const std::set<std::string> &vars)
  {
    initializedVariables = vars; // Copy initialized variables from SymbolTableVisitor
  }

  // Méthodes override pour la génération de code.
  virtual antlrcpp::Any visitProg(ifccParser::ProgContext* ctx) override;
  // Méthodes override pour la génération de code.
  virtual antlrcpp::Any visitProg(ifccParser::ProgContext* ctx) override;
  antlrcpp::Any visitStatement(ifccParser::StatementContext *ctx) override;
  antlrcpp::AnyvisitAssignment(ifccParser::AssignmentContext* ctx) override;
  antlrcpp::Any visitDeclaration(ifccParser::DeclarationContext *ctx) override;
  antlrcpp::Any visitOperand(ifccParser::OperandContext* ctx) override;
  antlrcpp::Any visitOperandExpr(ifccParser::OperandExprContext* ctx) override;
  antlrcpp::Any visitMulDiv(ifccParser::MulDivContext* ctx) override;
  antlrcpp::Any visitAddSub(ifccParser::AddSubContext* ctx) override;
  antlrcpp::Any visitParens(ifccParser::ParensContext* ctx) override;
  antlrcpp::Any visitBitOps(ifccParser::BitOpsContext *ctx);
  virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext* ctx) override;

  antlrcpp::Any visitFuncCall(ifccParser::FuncCallContext *ctx) override;

};
