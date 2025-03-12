#pragma once

#include "antlr4-runtime.h"
#include <unordered_map>
#include <iostream>
#include "generated/ifccBaseVisitor.h"

class CodeGenVisitor : public ifccBaseVisitor {
private:
  // La table des symboles stocke ici les offsets des variables (de type int)
  std::unordered_map<std::string, int> symbolTable;

public:
  // Copie la table des symboles depuis le visiteur de la table des symboles.
  void setSymbolTable(const std::unordered_map<std::string, int>& table) {
    symbolTable = table;
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

  // Nouvelle méthode pour gérer les appels de fonction.
  // Notez que le contexte est CallExpressionContext (généré par l'étiquette #CallExpression).
  antlrcpp::Any visitCallExpr(ifccParser::CallExprContext* ctx) override;
};
