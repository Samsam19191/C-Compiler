#include "CodeGenVisitorV2.h"
#include "IR.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>

using namespace std;

CodeGenVisitorV2::~CodeGenVisitorV2() {}

antlrcpp::Any CodeGenVisitorV2::visitProg(ifccParser::ProgContext *ctx)
{
    for (auto stmt : ctx->statement()) 
    { 
        visit(stmt); 
    }

    visit(ctx->return_stmt());
    return 0;
}

antlrcpp::Any CodeGenVisitorV2::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    visit(ctx->expr());
    return 0;
}

antlrcpp::Any CodeGenVisitorV2::visitAssignment(ifccParser::AssignmentContext *ctx)
{
    string varName = ctx->ID(0)->getText();
    if (symbolTable.find(varName) == symbolTable.end())
    {
        cerr << "Error: Undefined variable '" << varName
             << "' during code generation.\n";
        exit(1);
    }
    // Build the offset string using BP_REG placeholder.
    string offset = "-" + to_string(symbolTable[varName]);
    visit(ctx->expr(0));
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {ACC_REG, BP_REG, offset});
    return 0;
}

antlrcpp::Any CodeGenVisitorV2::visitOperand(ifccParser::OperandContext *ctx)
{
    if (ctx->CONSTINT())
    {
        int value = stoi(ctx->CONSTINT()->getText());
        cfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT, {ACC_REG, to_string(value)});
    }
    else if (ctx->CONSTCHAR())
    {
        string literal = ctx->CONSTCHAR()->getText();
        char c = literal[1];
        int value = static_cast<int>(c);
        cfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT, {ACC_REG, to_string(value)});
    }
    else if (ctx->ID())
    {
        string varName = ctx->ID()->getText();
        if (symbolTable.find(varName) == symbolTable.end())
        {
            cerr << "Error: Undefined variable '" << varName
                 << "' during code generation." << endl;
            exit(1);
        }
        // Convert the variable name to its offset string using BP_REG.
        string offset = "-" + to_string(symbolTable[varName]);
        cfg->current_bb->add_IRInstr(IRInstr::rmem, Type::INT, {ACC_REG, BP_REG, offset});
    }
    return 0;
}

antlrcpp::Any CodeGenVisitorV2::visitOperandExpr(ifccParser::OperandExprContext *ctx)
{
    return visit(ctx->operand());
}

antlrcpp::Any CodeGenVisitorV2::visitMulDiv(ifccParser::MulDivContext *ctx)
{
    visit(ctx->expr(1));
    string temp = cfg->create_new_tempvar(Type::INT);
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {temp, ACC_REG});
    visit(ctx->expr(0));
    if (ctx->getText().find("*") != string::npos)
    {
        cfg->current_bb->add_IRInstr(IRInstr::mul, Type::INT, {ACC_REG, ACC_REG, temp});
    }
    else
    {
        cfg->current_bb->add_IRInstr(IRInstr::call, Type::INT, {"div", ACC_REG, temp});
    }
    return 0;
}


antlrcpp::Any CodeGenVisitorV2::visitAddSub(ifccParser::AddSubContext *ctx) {
    // Évaluer le premier opérande (a) dans ACC_REG
    visit(ctx->expr(0));
    // Créer un temporaire déjà formaté, par exemple "-8(%rbp)"
    string temp_addr = cfg->create_new_tempvar(Type::INT);
    // Stocker la valeur de a dans le temporaire
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {ACC_REG, BP_REG, temp_addr});

    // Évaluer le second opérande (b) dans ACC_REG
    visit(ctx->expr(1));
    // Pour l'addition, ajouter la valeur stockée dans le temporaire (a) à b
    if (ctx->getText().find("+") != string::npos) {
        cfg->current_bb->add_IRInstr(IRInstr::add, Type::INT, {ACC_REG, ACC_REG, temp_addr});
    } else {
        // Pour la soustraction, on traite de la même manière (sans reformater)
        string tempB_addr = cfg->create_new_tempvar(Type::INT);
        cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {ACC_REG, BP_REG, tempB_addr});
        cfg->current_bb->add_IRInstr(IRInstr::sub, Type::INT, {ACC_REG, temp_addr, tempB_addr});
    }
    return 0;
}




antlrcpp::Any CodeGenVisitorV2::visitParens(ifccParser::ParensContext *ctx)
{
    return visit(ctx->expr());
}

antlrcpp::Any CodeGenVisitorV2::visitFuncCall(ifccParser::FuncCallContext *ctx)
{
    string functionName = ctx->ID()->getText();
    int argIndex = 0;
    // Replace with global register placeholders where applicable.
    vector<string> registers = {RDI_REG, "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    vector<string> argRegs;

    for (auto expr : ctx->expr()) {
        visit(expr);
        if (argIndex < registers.size()) {
            string reg = registers[argIndex];
            argRegs.push_back(reg);
        } else {
            cerr << "Error: Too many arguments for function '" << functionName << "'." << endl;
            exit(1);
        }
        argIndex++;
    }
    

    vector<string> callParams;
    callParams.push_back(functionName);
    callParams.push_back(ACC_REG);
    for (auto reg : argRegs)
    {
        callParams.push_back(reg);
    }
    cfg->current_bb->add_IRInstr(IRInstr::call, Type::INT, callParams);
    return 0;
}


antlrcpp::Any CodeGenVisitorV2::visitDeclaration(ifccParser::DeclarationContext *ctx)
{
    int n = ctx->ID().size();
    // Pour chaque variable déclarée
    for (int i = 0; i < n; i++) {
         // S'il y a une expression d'initialisation pour cette variable (i.e. i < nombre d'expressions)
         if (i < ctx->expr().size()) {
              // Évalue l'expression d'initialisation, le résultat est placé dans ACC_REG
              visit(ctx->expr(i));
              // Récupère le nom de la variable
              string varName = ctx->ID(i)->getText();
              // Construit le string d'offset à partir de la table des symboles (valeur positive)
              string offset = "-" + to_string(symbolTable[varName]);
              // Génére une instruction de store : copie la valeur de ACC_REG dans la mémoire à l'adresse offset(%rbp)
              cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {ACC_REG, BP_REG, offset});
         }
    }
    return 0;
}

