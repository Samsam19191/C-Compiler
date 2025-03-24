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
