#pragma once

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "generated/ifccParser.h"
#include <unordered_map>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "SymbolTableVisitor.h"

class CodeGenVisitor : public ifccBaseVisitor {
private:
  // Cette table est maintenant spécifique à la fonction en cours.
  std::unordered_map<std::string, FunctionSymbolTable> globalFuncTables;
  std::unordered_map<std::string, int> symbolTable;
  std::set<std::string> initializedVariables;
  std::string currentFunctionName;

public:
  // Permet de copier la table des symboles pour la fonction en cours, issue de SymbolTableVisitor.
  void setSymbolTable(const std::unordered_map<std::string, int>& table) {
    symbolTable = table;
  }
  void setInitializedVariables(const std::set<std::string>& vars) {
    initializedVariables = vars;
  }

  std::set<std::string> getInitializedVariables() const {
    return initializedVariables;
  }

  void setGlobalFuncTables(const std::unordered_map<std::string, FunctionSymbolTable>& tables) {
    globalFuncTables = tables;
  }

  // Méthodes override pour la nouvelle grammaire.
  virtual antlrcpp::Any visitProg(ifccParser::ProgContext* ctx) override;
  virtual antlrcpp::Any visitFunctionDefinition(ifccParser::FunctionDefinitionContext *ctx) override;
  virtual antlrcpp::Any visitBlock(ifccParser::BlockContext *ctx) override;
  virtual antlrcpp::Any visitDeclaration(ifccParser::DeclarationContext *ctx) override;
  virtual antlrcpp::Any visitAssignment(ifccParser::AssignmentContext *ctx) override;
  virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;
  virtual antlrcpp::Any visitIf_stmt(ifccParser::If_stmtContext *ctx) override;
  virtual antlrcpp::Any visitWhile_stmt(ifccParser::While_stmtContext *ctx) override;
  virtual antlrcpp::Any visitAddSub(ifccParser::AddSubContext *ctx) override;
  virtual antlrcpp::Any visitMulDiv(ifccParser::MulDivContext *ctx) override;
  virtual antlrcpp::Any visitParens(ifccParser::ParensContext *ctx) override;
  virtual antlrcpp::Any visitOperandExpr(ifccParser::OperandExprContext *ctx) override;
  virtual antlrcpp::Any visitOperand(ifccParser::OperandContext *ctx) override;
  virtual antlrcpp::Any visitFuncCall(ifccParser::FuncCallContext *ctx) override;
};
