#include "CodeGenVisitor.h"
#include "Type.h"
#include "VisitorErrorListener.h"
#include "IR.h"
#include "support/Any.h"
#include "Symbol.h"

#include <memory>
#include <string>

using namespace std;

CodeGenVisitor::CodeGenVisitor()
{
  shared_ptr<CFG> getchar =
      make_shared<CFG>(Type::INT, "getchar", 0, this);

  shared_ptr<CFG> putchar =
      make_shared<CFG>(Type::INT, "putchar", 0, this);
  auto symbol = putchar->add_parameter("c", Type::INT, 0);
  symbol->used = true;

  cfgList.push_back(getchar);
  functions["getchar"] = getchar;
  cfgList.push_back(putchar);
  functions["putchar"] = putchar;
}

antlrcpp::Any CodeGenVisitor::visitAxiom(ifccParser::AxiomContext *ctx)
{
  return visit(ctx->prog());
}

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx)
{
  for (ifccParser::FuncContext *func : ctx->func())
  {
    Type type;
    if (func->TYPE(0)->toString() == "int")
    {
      type = Type::INT;
    }
    else if (func->TYPE(0)->toString() == "char")
    {
      type = Type::CHAR;
    }
    else if (func->TYPE(0)->toString() == "void")
    {
      type = Type::VOID;
    }
    int argCount = func->ID().size() - 1;
    curCfg =
        make_shared<CFG>(type, func->ID(0)->toString(), argCount, this);
    cfgList.push_back(curCfg);
    functions[func->ID(0)->toString()] = curCfg;
    visit(func);
    curCfg->pop_table();
  }

  if (VisitorErrorListener::hasError())
  {
    exit(1);
  }

  cout << assembly.str();

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitFunc(ifccParser::FuncContext *ctx)
{
  for (int i = 1; i < ctx->ID().size(); i++)
  {
    Type type = (ctx->TYPE(i)->toString() == "int" ? Type::INT : Type::CHAR);
    auto symbol = curCfg->add_parameter(ctx->ID(i)->toString(), type,
                                        ctx->getStart()->getLine());
    curCfg->current_bb->add_IRInstr(IRInstr::param_decl, type, {symbol});
  }

  for (ifccParser::StmtContext *stmt : ctx->block()->stmt())
  {
    visit(stmt);
  }

  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitVar_decl_stmt(ifccParser::Var_decl_stmtContext *ctx)
{
  Type type;
  if (ctx->TYPE()->toString() == "int")
  {
    type = Type::INT;
  }
  else if (ctx->TYPE()->toString() == "char")
  {
    type = Type::CHAR;
  }
  else if (ctx->TYPE()->toString() == "void")
  {
    VisitorErrorListener::addError(ctx, "Can't create a variable of type void");
  }
  for (auto &memberCtx : ctx->var_decl_member())
  {
    string varName = memberCtx->ID()->toString();
    addSymbolToSymbolTableFromContext(memberCtx, varName, type);
    if (memberCtx->expr())
    {
      shared_ptr<Symbol> symbol = getSymbolFromSymbolTableByContext(memberCtx, varName);
      shared_ptr<Symbol> source = visit(memberCtx->expr()).as<shared_ptr<Symbol>>();
      curCfg->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {symbol, source});
    }
    else
    {
      // Initialisation implicite à 0
      shared_ptr<Symbol> symbol = getSymbolFromSymbolTableByContext(memberCtx, varName);
      auto zero = curCfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT, {"0"});
      curCfg->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {symbol, zero});
    }
  }
  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitVar_assign_stmt(ifccParser::Var_assign_stmtContext *ctx)
{
  shared_ptr<Symbol> symbol = getSymbolFromSymbolTableByContext(ctx, ctx->ID()->toString());

  if (symbol == nullptr)
  {
    return 1;
  }

  shared_ptr<Symbol> source =
      visit(ctx->expr()).as<shared_ptr<Symbol>>();

  // Récupérer l'opérateur directement à partir du texte brut
  std::string fullText = ctx->getText();
  size_t idLength = ctx->ID()->getText().length();
  size_t exprStart = fullText.find(ctx->expr()->getText());
  std::string op = fullText.substr(idLength, exprStart - idLength);

  if (op == "=")
  {
    curCfg->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {symbol, source});
  }
  else
  {
    IRInstr::Operation instr;
    if (op == "+=")
      instr = IRInstr::add;
    else if (op == "-=")
      instr = IRInstr::sub;
    else if (op == "*=")
      instr = IRInstr::mul;
    else if (op == "/=")
      instr = IRInstr::div;

    shared_ptr<Symbol> temp = curCfg->create_new_tempvar(Type::INT);
    curCfg->current_bb->add_IRInstr(IRInstr::ldvar, Type::INT, {symbol});
    curCfg->current_bb->add_IRInstr(instr, Type::INT, {symbol, source, temp});
    curCfg->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {symbol, temp});
  }
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitIf(ifccParser::IfContext *ctx)
{
  shared_ptr<Symbol> result =
      visit(ctx->expr()).as<shared_ptr<Symbol>>();
  string nextBBLabel = ".L" + to_string(nextLabel);
  nextLabel++;

  BasicBlock *baseBlock = curCfg->current_bb;
  BasicBlock *trueBlock = new BasicBlock(curCfg.get(), "");
  BasicBlock *falseBlock = new BasicBlock(curCfg.get(), nextBBLabel);

  baseBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result});

  trueBlock->exit_true = falseBlock;
  falseBlock->exit_true = baseBlock->exit_true;

  curCfg->add_bb(trueBlock);
  visit(ctx->block());

  curCfg->add_bb(falseBlock);

  baseBlock->exit_true = trueBlock;
  baseBlock->exit_false = falseBlock;
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitIf_else(ifccParser::If_elseContext *ctx)
{
  shared_ptr<Symbol> result =
      visit(ctx->expr()).as<shared_ptr<Symbol>>();
  string elseBBLabel = ".L" + to_string(nextLabel);
  nextLabel++;
  string endBBLabel = ".L" + to_string(nextLabel);
  nextLabel++;

  BasicBlock *baseBlock = curCfg->current_bb;
  BasicBlock *trueBlock = new BasicBlock(curCfg.get(), "");
  BasicBlock *elseBlock = new BasicBlock(curCfg.get(), elseBBLabel);
  BasicBlock *endBlock = new BasicBlock(curCfg.get(), endBBLabel);

  trueBlock->exit_true = endBlock;
  elseBlock->exit_true = endBlock;
  endBlock->exit_true = baseBlock->exit_true;

  baseBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result});

  curCfg->add_bb(trueBlock);
  visit(ctx->if_block);

  curCfg->add_bb(elseBlock);
  visit(ctx->else_block);

  curCfg->add_bb(endBlock);

  baseBlock->exit_true = trueBlock;
  baseBlock->exit_false = elseBlock;
  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitWhile_stmt(ifccParser::While_stmtContext *ctx)
{
  string conditionBBLabel = ".L" + to_string(nextLabel);
  nextLabel++;
  string endBBLabel = ".L" + to_string(nextLabel);
  nextLabel++;

  BasicBlock *baseBlock = curCfg->current_bb;
  BasicBlock *conditionBlock = new BasicBlock(curCfg.get(), conditionBBLabel);
  BasicBlock *stmtBlock = new BasicBlock(curCfg.get(), "");
  BasicBlock *endBlock = new BasicBlock(curCfg.get(), endBBLabel);

  conditionBlock->exit_true = stmtBlock;
  conditionBlock->exit_false = endBlock;
  endBlock->exit_true = baseBlock->exit_true;
  stmtBlock->exit_true = conditionBlock;
  baseBlock->exit_true = conditionBlock;

  curCfg->add_bb(conditionBlock);
  shared_ptr<Symbol> result =
      visit(ctx->expr()).as<shared_ptr<Symbol>>();
  conditionBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result});

  curCfg->add_bb(stmtBlock);
  visit(ctx->block());

  curCfg->add_bb(endBlock);

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitBlock(ifccParser::BlockContext *ctx)
{
  curCfg->push_table();
  for (ifccParser::StmtContext *stmt : ctx->stmt())
  {
    visit(stmt);
  }

  curCfg->pop_table();
  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
  if (curCfg->get_return_type() != Type::VOID)
  {
    if (ctx->expr() == nullptr)
    {
      string message =
          "Non void function " + curCfg->get_name() + " should return a value";
      VisitorErrorListener::addError(ctx, message);
      return 1;
    }
    shared_ptr<Symbol> val =
        visit(ctx->expr()).as<shared_ptr<Symbol>>();
    curCfg->current_bb->add_IRInstr(IRInstr::ret, curCfg->get_return_type(),
                                    {val});
  }
  else
  {
    if (ctx->expr() != nullptr)
    {
      string message =
          "Void function " + curCfg->get_name() + " should not return a value";
      VisitorErrorListener::addError(ctx, message);
      return 1;
    }
    curCfg->current_bb->add_IRInstr(IRInstr::ret, curCfg->get_return_type(),
                                    {});
  }

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitPar(ifccParser::ParContext *ctx)
{
  return visit(ctx->expr());
}

antlrcpp::Any
CodeGenVisitor::visitFunc_call(ifccParser::Func_callContext *ctx)
{
  auto it = functions.find(ctx->ID()->toString());
  if (it == functions.end())
  {
    string message =
        "Function " + ctx->ID()->toString() + " has not been declared";
    VisitorErrorListener::addError(ctx, message);
  }

  auto funcCfg = it->second;

  if (ctx->expr().size() != funcCfg->get_parameters_type().size())
  {
    string message = "Wrong number of parameters in function call to " +
                     funcCfg->get_name() + ": expected " +
                     to_string(funcCfg->get_parameters_type().size()) +
                     " but found " + to_string(ctx->expr().size()) +
                     " instead";
    VisitorErrorListener::addError(ctx, message);
  }

  vector<Parameter> params = {ctx->ID()->toString()};
  for (int i = 0; i < funcCfg->get_parameters_type().size(); i++)
  {
    shared_ptr<Symbol> symbol =
        visit(ctx->expr(i)).as<shared_ptr<Symbol>>();
    params.push_back(symbol);
    curCfg->current_bb->add_IRInstr(
        IRInstr::param, funcCfg->get_parameters_type()[i].type, {symbol});
  }

  return curCfg->current_bb->add_IRInstr(IRInstr::call,
                                         funcCfg->get_return_type(), params);
}

antlrcpp::Any CodeGenVisitor::visitMultdiv(ifccParser::MultdivContext *ctx)
{
  IRInstr::Operation instr;
  if (ctx->op->getText() == "*")
  {
    instr = IRInstr::mul;
  }
  else if (ctx->op->getText() == "/")
  {
    instr = IRInstr::div;
  }
  else
  {
    instr = IRInstr::mod;
  }

  shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<shared_ptr<Symbol>>();

  return curCfg->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitAddsub(ifccParser::AddsubContext *ctx)
{
  IRInstr::Operation instr =
      (ctx->op->getText() == "+" ? IRInstr::add : IRInstr::sub);

  shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<shared_ptr<Symbol>>();

  if (leftVal == nullptr || rightVal == nullptr)
  {
    VisitorErrorListener::addError(
        ctx, "Invalid operation with function returning void");
  }

  return curCfg->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitCmp(ifccParser::CmpContext *ctx)
{
  IRInstr::Operation instr;
  if (ctx->op->getText() == ">")
  {
    instr = IRInstr::Operation::gt;
  }
  else if (ctx->op->getText() == ">=")
  {
    instr = IRInstr::Operation::geq;
  }
  else if (ctx->op->getText() == "<")
  {
    instr = IRInstr::Operation::lt;
  }
  else if (ctx->op->getText() == "<=")
  {
    instr = IRInstr::Operation::leq;
  }

  shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<shared_ptr<Symbol>>();

  if (leftVal == nullptr || rightVal == nullptr)
  {
    VisitorErrorListener::addError(
        ctx, "Invalid operation with function returning void");
  }

  return curCfg->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitEq(ifccParser::EqContext *ctx)
{
  IRInstr::Operation instr =
      (ctx->op->getText() == "==" ? IRInstr::eq : IRInstr::neq);

  shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<shared_ptr<Symbol>>();

  if (leftVal == nullptr || rightVal == nullptr)
  {
    VisitorErrorListener::addError(
        ctx, "Invalid operation with function returning void");
  }

  return curCfg->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitVal(ifccParser::ValContext *ctx)
{
  shared_ptr<Symbol> source;
  if (ctx->ID() != nullptr)
  {
    shared_ptr<Symbol> symbol = getSymbolFromSymbolTableByContext(ctx, ctx->ID()->toString());
    if (symbol != nullptr)
    {
      source =
          curCfg->current_bb->add_IRInstr(IRInstr::ldvar, Type::INT, {symbol});
    }
  }
  else if (ctx->INTEGER_LITERAL() != nullptr)
  {
    source = curCfg->current_bb->add_IRInstr(
        IRInstr::ldconst, Type::INT, {ctx->INTEGER_LITERAL()->toString()});
  }
  else if (ctx->CHAR_LITERAL() != nullptr)
  {
    string val =
        to_string(static_cast<int>(ctx->CHAR_LITERAL()->toString()[1]));
    source =
        curCfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::CHAR, {val});
  }

  return source;
}

bool CodeGenVisitor::addSymbolToSymbolTableFromContext(antlr4::ParserRuleContext *ctx,
                                                       const string &id, Type type)
{
  bool result = curCfg->add_symbol(id, type, ctx->getStart()->getLine());
  if (!result)
  {
    string error = "The variable " + id + " has already been declared";
    VisitorErrorListener::addError(ctx, error, ErrorType::Error);
  }
  return result;
}

shared_ptr<Symbol>
CodeGenVisitor::getSymbolFromSymbolTableByContext(antlr4::ParserRuleContext *ctx,
                                                  const string &id)
{
  shared_ptr<Symbol> symbol = curCfg->get_symbol(id);
  if (symbol == nullptr)
  {
    const string error = "Symbol not found: " + id;
    VisitorErrorListener::addError(ctx, error, ErrorType::Error);
    return nullptr;
  }

  symbol->used = true;
  return symbol;
}

antlrcpp::Any CodeGenVisitor::visitB_and(ifccParser::B_andContext *ctx)
{
  shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<shared_ptr<Symbol>>();

  return curCfg->current_bb->add_IRInstr(IRInstr::b_and, Type::INT,
                                         {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitB_or(ifccParser::B_orContext *ctx)
{
  shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<shared_ptr<Symbol>>();

  if (leftVal == nullptr || rightVal == nullptr)
  {
    VisitorErrorListener::addError(
        ctx, "Invalid operation with function returning void");
  }

  return curCfg->current_bb->add_IRInstr(IRInstr::b_or, Type::INT,
                                         {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitB_xor(ifccParser::B_xorContext *ctx)
{
  shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<shared_ptr<Symbol>>();

  if (leftVal == nullptr || rightVal == nullptr)
  {
    VisitorErrorListener::addError(
        ctx, "Invalid operation with function returning void");
  }

  return curCfg->current_bb->add_IRInstr(IRInstr::b_xor, Type::INT,
                                         {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitUnaryOp(ifccParser::UnaryOpContext *ctx)
{
  shared_ptr<Symbol> val =
      visit(ctx->expr()).as<shared_ptr<Symbol>>();
  IRInstr::Operation instr;
  if (ctx->op->getText() == "-")
  {
    instr = IRInstr::neg;
    return curCfg->current_bb->add_IRInstr(instr, Type::INT, {val});
  }
  else if (ctx->op->getText() == "~")
  {
    instr = IRInstr::not_;
    return curCfg->current_bb->add_IRInstr(instr, Type::INT, {val});
  }
  else if (ctx->op->getText() == "!")
  {
    instr = IRInstr::lnot;
    return curCfg->current_bb->add_IRInstr(instr, Type::INT, {val});
  }
  else if (ctx->op->getText() == "++")
  {
    instr = IRInstr::inc;
    return curCfg->current_bb->add_IRInstr(instr, Type::INT, {val});
  }
  else if (ctx->op->getText() == "--")
  {
    instr = IRInstr::dec;
    return curCfg->current_bb->add_IRInstr(instr, Type::INT, {val});
  }
  else if (ctx->op->getText() == "+")
  {
    return val;
  }
  return nullptr;
}

antlrcpp::Any CodeGenVisitor::visitPostIncDec(ifccParser::PostIncDecContext *ctx)
{
  shared_ptr<Symbol> symbol = getSymbolFromSymbolTableByContext(ctx, ctx->ID()->getText());

  // Récupérer l'opérateur à partir du texte brut
  std::string fullText = ctx->getText();
  std::string op = fullText.substr(fullText.size() - 2); // Les deux derniers caractères

  IRInstr::Operation operation = (op == "++") ? IRInstr::inc : IRInstr::dec;

  // Sauvegarde de la valeur avant incrémentation
  shared_ptr<Symbol> temp = curCfg->create_new_tempvar(Type::INT);
  curCfg->current_bb->add_IRInstr(IRInstr::ldvar, Type::INT, {symbol});
  curCfg->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {temp, symbol});

  // Incrémentation
  curCfg->current_bb->add_IRInstr(operation, Type::INT, {symbol});

  return temp;
}

antlrcpp::Any CodeGenVisitor::visitPreIncDec(ifccParser::PreIncDecContext *ctx)
{
  shared_ptr<Symbol> symbol = getSymbolFromSymbolTableByContext(ctx, ctx->ID()->getText());

  // Récupérer l'opérateur à partir du texte brut
  std::string fullText = ctx->getText();
  std::string op = fullText.substr(0, 2); // Les deux premiers caractères

  IRInstr::Operation operation = (op == "++") ? IRInstr::inc : IRInstr::dec;

  // Incrémentation avant utilisation
  curCfg->current_bb->add_IRInstr(operation, Type::INT, {symbol});

  return symbol;
}

antlrcpp::Any CodeGenVisitor::visitLogicalOr(ifccParser::LogicalOrContext *ctx) {
  string falseLabel = ".L" + to_string(nextLabel++);
  string endLabel = ".L" + to_string(nextLabel++);
  
  shared_ptr<Symbol> left = visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> result = curCfg->create_new_tempvar(Type::INT);
  
  // Évaluation paresseuse - si gauche est vrai, résultat est vrai
  curCfg->current_bb->add_IRInstr(IRInstr::cmpNZ, Type::INT, {left});
  curCfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT, {"1"});
  curCfg->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {result, left});
  
  BasicBlock* falseBB = new BasicBlock(curCfg.get(), falseLabel);
  BasicBlock* endBB = new BasicBlock(curCfg.get(), endLabel);
  
  curCfg->current_bb->exit_false = falseBB;
  curCfg->current_bb->exit_true = endBB;
  
  curCfg->add_bb(falseBB);
  shared_ptr<Symbol> right = visit(ctx->expr(1)).as<shared_ptr<Symbol>>();
  curCfg->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {result, right});
  
  curCfg->add_bb(endBB);
  return result;
}

antlrcpp::Any CodeGenVisitor::visitLogicalAnd(ifccParser::LogicalAndContext *ctx) {
  string trueLabel = ".L" + to_string(nextLabel++);
  string endLabel = ".L" + to_string(nextLabel++);
  
  shared_ptr<Symbol> left = visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> result = curCfg->create_new_tempvar(Type::INT);
  
  // Évaluation paresseuse - si gauche est faux, résultat est faux
  curCfg->current_bb->add_IRInstr(IRInstr::cmpNZ, Type::INT, {left});
  curCfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT, {"0"});
  curCfg->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {result, left});
  
  BasicBlock* trueBB = new BasicBlock(curCfg.get(), trueLabel);
  BasicBlock* endBB = new BasicBlock(curCfg.get(), endLabel);
  
  curCfg->current_bb->exit_true = trueBB;
  curCfg->current_bb->exit_false = endBB;
  
  curCfg->add_bb(trueBB);
  shared_ptr<Symbol> right = visit(ctx->expr(1)).as<shared_ptr<Symbol>>();
  curCfg->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {result, right});
  
  curCfg->add_bb(endBB);
  return result;
}