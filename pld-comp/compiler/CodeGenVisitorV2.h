#pragma once

#include "antlr4-runtime.h"
#include <unordered_map>
#include <iostream>
#include "generated/ifccBaseVisitor.h"
#include "IR.h"  // On utilise désormais le module IR

class CodeGenVisitor : public ifccBaseVisitor {
private:
  // La table des symboles stocke ici les offsets des variables (de type int)
  std::unordered_map<std::string, int> symbolTable;
  CFG* cfg; // Pointeur vers le CFG où l'on construit l'IR

public:
  // Copie la table des symboles depuis le visiteur de la table des symboles.
  void setSymbolTable(const std::unordered_map<std::string, int>& table) {
    symbolTable = table;
  }
  
  // Permet de fixer le CFG (créé dans main.cpp ou ailleurs)
  void setCFG(CFG* c) {
    cfg = c;
  }

  // Méthodes override pour la génération de code.
  virtual antlrcpp::Any visitProg(ifccParser::ProgContext* ctx) override;
  antlrcpp::Any visitAssignment(ifccParser::AssignmentContext* ctx) override;
  antlrcpp::Any visitOperand(ifccParser::OperandContext* ctx) override;
  antlrcpp::Any visitOperandExpr(ifccParser::OperandExprContext* ctx) override;
  antlrcpp::Any visitMulDiv(ifccParser::MulDivContext* ctx) override;
  antlrcpp::Any visitAddSub(ifccParser::AddSubContext* ctx) override;
  antlrcpp::Any visitParens(ifccParser::ParensContext* ctx) override;
  virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext* ctx) override;
  antlrcpp::Any visitFuncCall(ifccParser::FuncCallContext *ctx) override;
};
