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
    for (auto assignment : ctx->assignment())
    {
        visit(assignment);
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
    string varName = ctx->ID()->getText();
    if (symbolTable.find(varName) == symbolTable.end())
    {
        cerr << "Error: Undefined variable '" << varName
             << "' during code generation.\n";
        exit(1);
    }
    // Build the offset string using BP_REG placeholder.
    string offset = to_string(symbolTable[varName]) + "(" + BP_REG + ")";
    visit(ctx->expr());
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {offset, ACC_REG});
    return 0;
}

antlrcpp::Any CodeGenVisitorV2::visitOperand(ifccParser::OperandContext *ctx)
{
    if (ctx->CONST())
    {
        int value = stoi(ctx->CONST()->getText());
        cfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT, {ACC_REG, to_string(value)});
    }
    else if (ctx->CHAR())
    {
        string literal = ctx->CHAR()->getText();
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
        string offset = to_string(symbolTable[varName]) + "(" + BP_REG + ")";
        cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {ACC_REG, offset});
    }
    return 0;
}

antlrcpp::Any CodeGenVisitorV2::visitOperandExpr(ifccParser::OperandExprContext *ctx)
{
    return visit(ctx->operand());
}

antlrcpp::Any CodeGenVisitorV2::visitMulDiv(ifccParser::MulDivContext *ctx)
{
    if (ctx->getText().find("*") != string::npos)
    {
        // Évaluation de l'opérande gauche (premier facteur) dans %eax
        visit(ctx->expr(0));
        
        // Sauvegarde de l'opérande gauche dans une variable temporaire
        string temp = cfg->create_new_tempvar(Type::INT);
        cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {temp, "%eax"});
        
        // Évaluation de l'opérande droite (second facteur) dans %eax
        visit(ctx->expr(1));
        
        // Génération de l'instruction de multiplication :
        // Le résultat de l'opérande droite est dans %eax, et l'opérande gauche est sauvegardée dans temp.
        // L'instruction générée sera, par exemple, "movl temp, %eax" suivie de "imull %eax, %eax",
        // ce qui donne %eax = (%eax) * (valeur sauvegardée dans temp)
        cfg->current_bb->add_IRInstr(IRInstr::mul, Type::INT, {"%eax", "%eax", temp});
    }
    else
    {
        // Division
        // 1. Évaluer l'opérande gauche (dividende) dans %eax
        visit(ctx->expr(0));
        // Sauvegarder le dividende dans une variable temporaire
        string tempDividend = cfg->create_new_tempvar(Type::INT);
        cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {tempDividend, "%eax"});
        
        // 2. Évaluer l'opérande droite (diviseur) dans %eax
        visit(ctx->expr(1));
        // Sauvegarder le diviseur dans une autre variable temporaire
        string tempDivisor = cfg->create_new_tempvar(Type::INT);
        cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {tempDivisor, "%eax"});
        
        // 3. Restaurer le dividende dans %eax
        cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {"%eax", tempDividend});
        
        // 4. Générer l'instruction de division avec le diviseur sauvegardé
        // L'opération div_op devra générer "cltd" suivi de "idivl <adresse_du_diviseur>"
        cfg->current_bb->add_IRInstr(IRInstr::div_op, Type::INT, {tempDivisor});
    }
    return 0;
}


antlrcpp::Any CodeGenVisitorV2::visitAddSub(ifccParser::AddSubContext *ctx)
{
    // Evaluate left operand (a) into ACC_REG
    visit(ctx->expr(0));
    string temp = cfg->create_new_tempvar(Type::INT);
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {temp, ACC_REG});
    // Evaluate right operand (b) into ACC_REG
    visit(ctx->expr(1));
    if (ctx->getText().find("+") != string::npos)
    {
        cfg->current_bb->add_IRInstr(IRInstr::add, Type::INT, {ACC_REG, ACC_REG, temp});
    }
    else
    {
        string tempB = cfg->create_new_tempvar(Type::INT);
        cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {tempB, ACC_REG});
        cfg->current_bb->add_IRInstr(IRInstr::sub, Type::INT, {ACC_REG, temp, tempB});
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

    for (auto expr : ctx->expr())
    {
        visit(expr);
        if (argIndex < registers.size())
        {
            string reg = registers[argIndex];
            cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {reg, ACC_REG});
            argRegs.push_back(reg);
        }
        else
        {
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
