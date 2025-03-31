#ifndef SYMBOLTABLEVISITOR_H
#define SYMBOLTABLEVISITOR_H

#include "generated/ifccBaseVisitor.h"
#include "generated/ifccParser.h"
#include <iostream>
#include <set>
#include <unordered_map>
#include <string>
#include <vector>

// Structure pour stocker la table des symboles d'une fonction
struct FunctionSymbolTable {
    std::unordered_map<std::string, int> symbols;       // Variable -> offset
    std::set<std::string> initializedVariables;         // Variables initialisées
};

class SymbolTableVisitor : public ifccBaseVisitor {
private:
  // Table globale pour chaque fonction : nom de la fonction -> table des symboles
  std::unordered_map<std::string, FunctionSymbolTable> functionSymbolTables;

  // Table des symboles courante (locale à la fonction en cours)
  std::unordered_map<std::string, int> currentSymbolTable;
  std::set<std::string> currentInitializedVariables;
  int currentOffset = 0;

public:

     std::unordered_map<std::string, FunctionSymbolTable> getFunctionSymbolTables (std::unordered_map<std::string, FunctionSymbolTable>& tables){
        return functionSymbolTables;
     }

    std::set<std::string> getInitializedVariables() const {
        return currentInitializedVariables;
    }

    // Visiteur pour les définitions de fonctions
    virtual antlrcpp::Any visitFunctionDefinition(ifccParser::FunctionDefinitionContext *ctx) override;
    virtual antlrcpp::Any visitBlock(ifccParser::BlockContext *ctx) override;
    virtual antlrcpp::Any visitDeclaration(ifccParser::DeclarationContext *ctx) override;
    virtual antlrcpp::Any visitAssignment(ifccParser::AssignmentContext *ctx) override;
    virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;
    virtual antlrcpp::Any visitIf_stmt(ifccParser::If_stmtContext *ctx) override;
    virtual antlrcpp::Any visitWhile_stmt(ifccParser::While_stmtContext *ctx) override;
    virtual antlrcpp::Any visitOperandExpr(ifccParser::OperandExprContext *ctx) override;
    virtual antlrcpp::Any visitOperand(ifccParser::OperandContext *ctx) override;
    virtual antlrcpp::Any visitFuncCall(ifccParser::FuncCallContext *ctx) override;

    // Vérification finale des variables déclarées mais jamais utilisées pour la fonction en cours.
    void checkUnusedVariables();

    // Accesseurs pour récupérer les tables de symboles par fonction
    const std::unordered_map<std::string, FunctionSymbolTable>& getFunctionSymbolTables() const {
      return functionSymbolTables;
    }
};

#endif // SYMBOLTABLEVISITOR_H
