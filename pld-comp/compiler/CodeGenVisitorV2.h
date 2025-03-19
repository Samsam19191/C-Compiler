#pragma once

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "IR.h"
#include <unordered_map>
#include <string>

using namespace std;

class CodeGenVisitorV2 : public ifccBaseVisitor {
public:
    // Constructeur et destructeur virtuel (pour générer la vtable)
    CodeGenVisitorV2() : cfg(nullptr) {}
    virtual ~CodeGenVisitorV2();

    // Méthode pour fixer la table des symboles
    void setSymbolTable(const unordered_map<string,int>& table) {
        symbolTable = table;
    }
    
    // Méthode pour fixer le CFG utilisé pour construire l'IR
    void setCFG(CFG* c) {
        cfg = c;
    }

    // Méthodes de visite override pour construire l'IR
    virtual antlrcpp::Any visitProg(ifccParser::ProgContext* ctx) override;
    antlrcpp::Any visitAssignment(ifccParser::AssignmentContext* ctx) override;
    virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext* ctx) override;
    antlrcpp::Any visitOperand(ifccParser::OperandContext* ctx) override;
    antlrcpp::Any visitOperandExpr(ifccParser::OperandExprContext* ctx) override;
    antlrcpp::Any visitMulDiv(ifccParser::MulDivContext* ctx) override;
    antlrcpp::Any visitAddSub(ifccParser::AddSubContext* ctx) override;
    antlrcpp::Any visitParens(ifccParser::ParensContext* ctx) override;
    antlrcpp::Any visitFuncCall(ifccParser::FuncCallContext* ctx) override;
    
private:
    unordered_map<string,int> symbolTable;
    CFG* cfg;
};
