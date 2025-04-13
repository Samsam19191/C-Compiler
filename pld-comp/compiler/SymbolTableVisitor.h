#ifndef SYMBOLTABLEVISITOR_H
#define SYMBOLTABLEVISITOR_H

#include "generated/ifccBaseVisitor.h"
#include <iostream>
#include <set>
#include <unordered_map>

using namespace std;

class SymbolTableVisitor : public ifccBaseVisitor {
private:
  unordered_map<string, int>
      symbolTable;                     // Variable name -> stack offset
  set<string> usedVariables; // Variables that were accessed
  int currentOffset = -4; // Start at -4(%rbp), move down for each variable

public:
  antlrcpp::Any visitAssignment(ifccParser::AssignmentContext *ctx) override;
  antlrcpp::Any visitOperand(ifccParser::OperandContext *ctx) override;
  void checkUnusedVariables();
  unordered_map<string, int> getSymbolTable() { return symbolTable; }
  antlrcpp::Any visitFuncCall(ifccParser::FuncCallContext *ctx) override;
  antlrcpp!!Any visitVar_decl_stmt(ifccParser::Var_decl_stmtContext *ctx) override;

};

#endif // SYMBOLTABLEVISITOR_H