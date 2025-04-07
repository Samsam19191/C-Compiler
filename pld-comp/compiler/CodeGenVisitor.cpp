#include "CodeGenVisitor.h"
#include "IR.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx)
{
  // Création du bloc d'entrée du CFG
  std::string entryLabel = cfg->new_BB_name();
  BasicBlock *entryBB = new BasicBlock(cfg, entryLabel);
  cfg->add_bb(entryBB);
  cfg->current_bb = entryBB;

  // Traitement des instructions du programme
  for (auto statement : ctx->statement())
  {
    visit(statement);
  }

  // Traitement de l'instruction return
  std::string retVal = visit(ctx->return_stmt()).as<std::string>();

  // Ajouter la variable spéciale "!retvalue" dans la table des symboles
  cfg->add_to_symbol_table("!retvalue", Type::INT);

  // Générer l'instruction IR qui copie la valeur de retour dans "!retvalue"
  cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {"!retvalue", retVal});
  cfg->current_bb->exit_true = nullptr;

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitStatement(ifccParser::StatementContext *ctx)
{
  if (ctx->declaration())
    return visit(ctx->declaration());
  else if (ctx->assignment())
    return visit(ctx->assignment());
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
  return visit(ctx->expr());
}

antlrcpp::Any CodeGenVisitor::visitAssignment(ifccParser::AssignmentContext *ctx)
{
  const vector<antlr4::tree::TerminalNode *> &varNames = ctx->ID();
  const vector<ifccParser::ExprContext *> &exprs = ctx->expr();

  for (int i = 0; i < varNames.size(); i++)
  {
    string varName = varNames[i]->getText();
    if (symbolTable.find(varName) == symbolTable.end())
    {
      std::cerr << "Error: Undefined variable '" << varName << "' during IR generation.\n";
      exit(1);
    }
    string exprResult = visit(exprs[i]).as<string>();
    initializedVariables.insert(varName);
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {varName, exprResult});
  }
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclaration(ifccParser::DeclarationContext *ctx)
{
  vector<ifccParser::ExprContext *> exprs = ctx->expr();
  if (!exprs.empty())
  {
    vector<antlr4::tree::TerminalNode *> varNames = ctx->ID();
    for (int i = 0; i < varNames.size(); i++)
    {
      string varName = varNames[i]->getText();
      if (symbolTable.find(varName) == symbolTable.end())
      {
        std::cerr << "Error: Undefined variable '" << varName << "' during IR generation.\n";
        exit(1);
      }
      string exprResult = visit(exprs[i]).as<string>();
      cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {varName, exprResult});
    }
  }
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitBitOps(ifccParser::BitOpsContext *ctx)
{
  string left = visit(ctx->expr(0)).as<string>();
  string right = visit(ctx->expr(1)).as<string>();
  string temp = cfg->create_new_tempvar(Type::INT);
  string op = ctx->op->getText();
  // Placeholder pour bit-ops
  cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {temp, left});
  return temp;
}

antlrcpp::Any CodeGenVisitor::visitOperand(ifccParser::OperandContext *ctx)
{
  if (ctx->CONSTINT())
    return ctx->CONSTINT()->getText();
  else if (ctx->CONSTCHAR())
  {
    string literal = ctx->CONSTCHAR()->getText();
    int value = static_cast<int>(literal[1]);
    return to_string(value);
  }
  else if (ctx->ID())
  {
    string varName = ctx->ID()->getText();
    if (symbolTable.find(varName) == symbolTable.end() || initializedVariables.find(varName) == initializedVariables.end())
    {
      std::cerr << "Error: Undefined or uninitialized variable '" << varName << "' during IR generation.\n";
      exit(1);
    }
    return varName;
  }
  return "";
}

antlrcpp::Any CodeGenVisitor::visitOperandExpr(ifccParser::OperandExprContext *ctx)
{
  return visit(ctx->operand());
}

antlrcpp::Any CodeGenVisitor::visitMulDiv(ifccParser::MulDivContext *ctx)
{
  string op = ctx->op->getText();
  string left = visit(ctx->expr(0)).as<string>();
  string right = visit(ctx->expr(1)).as<string>();
  string temp = cfg->create_new_tempvar(Type::INT);
  if (op == "*")
    cfg->current_bb->add_IRInstr(IRInstr::mul, Type::INT, {temp, left, right});
  else if (op == "/")
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {temp, left});
  else if (op == "%")
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {temp, left});
  return temp;
}

antlrcpp::Any CodeGenVisitor::visitAddSub(ifccParser::AddSubContext *ctx)
{
  string op = ctx->op->getText();
  string left = visit(ctx->expr(0)).as<string>();
  string right = visit(ctx->expr(1)).as<string>();
  string temp = cfg->create_new_tempvar(Type::INT);
  if (op == "+")
    cfg->current_bb->add_IRInstr(IRInstr::add, Type::INT, {temp, left, right});
  else
    cfg->current_bb->add_IRInstr(IRInstr::sub, Type::INT, {temp, left, right});
  return temp;
}

antlrcpp::Any CodeGenVisitor::visitParens(ifccParser::ParensContext *ctx)
{
  return visit(ctx->expr());
}

antlrcpp::Any CodeGenVisitor::visitFuncCall(ifccParser::FuncCallContext *ctx)
{
  string functionName = ctx->ID()->getText();
  vector<string> args;
  for (auto expr : ctx->expr())
  {
    string arg = visit(expr).as<string>();
    args.push_back(arg);
  }
  string temp = cfg->create_new_tempvar(Type::INT);
  vector<string> params;
  params.push_back(functionName);
  params.push_back(temp);
  for (auto &a : args)
    params.push_back(a);
  cfg->current_bb->add_IRInstr(IRInstr::call, Type::INT, params);
  return temp;
}
