#include "CodeGenVisitorV2.h"
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
        std::cerr << "Error: Undefined variable '" << varName
                  << "' during code generation.\n";
        exit(1);
    }
    string offset = to_string(symbolTable[varName]) + "(%rbp)";
    visit(ctx->expr());
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {offset, "%eax"});
    return 0;
}

antlrcpp::Any CodeGenVisitorV2::visitOperand(ifccParser::OperandContext *ctx)
{
    if (ctx->CONST())
    {
        int value = stoi(ctx->CONST()->getText());
        cfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT, {"%eax", to_string(value)});
    }
    else if (ctx->CHAR())
    {
        string literal = ctx->CHAR()->getText();
        char c = literal[1];
        int value = static_cast<int>(c);
        cfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT, {"%eax", to_string(value)});
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
        // Convert the variable name to its offset string (e.g., "-4(%rbp)")
        string offset = to_string(symbolTable[varName]) + "(%rbp)";
        cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {"%eax", offset});
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
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {temp, "%eax"});
    visit(ctx->expr(0));
    if (ctx->getText().find("*") != string::npos)
    {
        cfg->current_bb->add_IRInstr(IRInstr::mul, Type::INT, {"%eax", "%eax", temp});
    }
    else
    {
        cfg->current_bb->add_IRInstr(IRInstr::call, Type::INT, {"div", "%eax", temp});
    }
    return 0;
}

antlrcpp::Any CodeGenVisitorV2::visitAddSub(ifccParser::AddSubContext *ctx)
{
    visit(ctx->expr(0));
    string temp = cfg->create_new_tempvar(Type::INT);
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {temp, "%eax"});
    visit(ctx->expr(1));
    if (ctx->getText().find("+") != string::npos)
    {
        cfg->current_bb->add_IRInstr(IRInstr::add, Type::INT, {"%eax", temp, "%eax"});
    }
    else
    {
        cfg->current_bb->add_IRInstr(IRInstr::sub, Type::INT, {"%eax", temp, "%eax"});
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
    vector<string> registers = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    vector<string> argRegs;

    for (auto expr : ctx->expr())
    {
        visit(expr);
        if (argIndex < registers.size())
        {
            string reg = registers[argIndex];
            cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {reg, "%eax"});
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
    callParams.push_back("%eax");
    for (auto reg : argRegs)
    {
        callParams.push_back(reg);
    }
    cfg->current_bb->add_IRInstr(IRInstr::call, Type::INT, callParams);
    return 0;
}
