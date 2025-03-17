#ifndef SYMBOLTABLEVISITOR_H
#define SYMBOLTABLEVISITOR_H

#include "generated/ifccBaseVisitor.h"
#include <iostream>
#include <set>
#include <unordered_map>

class SymbolTableVisitor : public ifccBaseVisitor {
private:
  std::unordered_map<std::string, int>
      symbolTable;                     // Variable name -> stack offset
  std::set<std::string> usedVariables; // Variables that were accessed
  int currentOffset = 0; // Start at (%rbp), move down for each variable
  std::set<std::string> initializedVariables; // Variables that were initialized

      public :
      // antlrcpp::Any visitAssignment(ifccParser::AssignmentContext *ctx)
      // override;
      antlrcpp::Any
      visitDeclaration(ifccParser::DeclarationContext *ctx) override;
  antlrcpp::Any visitOperandExpr(ifccParser::OperandExprContext *ctx) override;
  antlrcpp::Any visitOperand(ifccParser::OperandContext *ctx) override;
  void checkUnusedVariables();
  std::unordered_map<std::string, int> getSymbolTable() { return symbolTable; }
  std::set<std::string> getInitializedVariables() { return initializedVariables; }
  antlrcpp::Any visitFuncCall(ifccParser::FuncCallContext *ctx) override;

};

#endif // SYMBOLTABLEVISITOR_H
