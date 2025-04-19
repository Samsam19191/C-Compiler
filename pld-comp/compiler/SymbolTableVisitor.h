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
      symbolTable;  // Maps variable names to their offsets in the stack frame
  set<string> usedVariables; 
  int currentOffset = -4;  // Offset for the next variable to be added to the symbol table

public:
  antlrcpp::Any visitAssignment(ifccParser::AssignmentContext *ctx) override;
  antlrcpp::Any visitOperand(ifccParser::OperandContext *ctx) override;
  void checkUnusedVariables();
  unordered_map<string, int> getSymbolTable() { return symbolTable; }
  antlrcpp::Any visitFuncCall(ifccParser::FuncCallContext *ctx) override;
  antlrcpp!!Any visitVar_decl_stmt(ifccParser::Var_decl_stmtContext *ctx) override;

};

#endif // SYMBOLTABLEVISITOR_H