#include "CodeGenVisitor.h"
#include "Type.h"
#include "VisitorErrorListener.h"
#include "IR.h"
#include "support/Any.h"
#include "Symbol.h"

#include <memory>
#include <string>

using namespace std;

// Constructeur de la classe CodeGenVisitor
CodeGenVisitor::CodeGenVisitor()
{
  // Création d'une fonction getchar avec un type de retour INT
  shared_ptr<CFG> getchar =
      make_shared<CFG>(Type::INT, "getchar", 0, this);

  // Création d'une fonction putchar avec un type de retour INT
  shared_ptr<CFG> putchar =
      make_shared<CFG>(Type::INT, "putchar", 0, this);

  // Ajout d'un paramètre "c" de type INT à la fonction putchar
  auto symbol = putchar->add_parameter("c", Type::INT, 0);
  symbol->used = true; // Marque le symbole comme utilisé

  // Ajout des fonctions getchar et putchar à la liste des CFG et au dictionnaire des fonctions
  cfgList.push_back(getchar);
  functions["getchar"] = getchar;
  cfgList.push_back(putchar);
  functions["putchar"] = putchar;
}

// Visite du nœud Axiom dans l'arbre syntaxique
antlrcpp::Any CodeGenVisitor::visitAxiom(ifccParser::AxiomContext *ctx)
{
  // Appelle la visite du nœud prog
  return visit(ctx->prog());
}

// Visite du nœud Prog dans l'arbre syntaxique
antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx)
{
  // Parcourt toutes les fonctions définies dans le programme
  for (ifccParser::FuncContext *func : ctx->func())
  {
    Type type;
    // Détermine le type de retour de la fonction
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

    // Crée une nouvelle CFG pour la fonction
    int argCount = func->ID().size() - 1;
    curCfg =
        make_shared<CFG>(type, func->ID(0)->toString(), argCount, this);

    // Ajoute la CFG à la liste et au dictionnaire des fonctions
    cfgList.push_back(curCfg);
    functions[func->ID(0)->toString()] = curCfg;

    // Visite le contenu de la fonction
    visit(func);

    // Nettoie la table des symboles
    curCfg->pop_table();
  }

  // Si des erreurs ont été détectées, termine le programme
  if (VisitorErrorListener::hasError())
  {
    exit(1);
  }

  // Affiche le code assembleur généré
  cout << assembly.str();

  return 0;
}

// Visite du nœud Func dans l'arbre syntaxique
antlrcpp::Any CodeGenVisitor::visitFunc(ifccParser::FuncContext *ctx)
{
  // Ajoute les paramètres de la fonction à la table des symboles
  for (int i = 1; i < ctx->ID().size(); i++)
  {
    Type type = (ctx->TYPE(i)->toString() == "int" ? Type::INT : Type::CHAR);
    auto symbol = curCfg->add_parameter(ctx->ID(i)->toString(), type,
                                        ctx->getStart()->getLine());
    curCfg->current_bb->add_IRInstr(IRInstr::param_decl, type, {symbol});
  }

  // Visite les instructions du bloc de la fonction
  for (ifccParser::StmtContext *stmt : ctx->block()->stmt())
  {
    visit(stmt);
  }

  return 0;
}

// Visite du nœud Var_decl_stmt pour déclarer des variables
antlrcpp::Any
CodeGenVisitor::visitVar_decl_stmt(ifccParser::Var_decl_stmtContext *ctx)
{
  Type type;
  // Détermine le type de la variable
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
    // Erreur : une variable ne peut pas être de type void
    VisitorErrorListener::addError(ctx, "Can't create a variable of type void");
  }

  // Parcourt les membres de la déclaration
  for (auto &memberCtx : ctx->var_decl_member())
  {
    string varName = memberCtx->ID()->toString();
    // Ajoute la variable à la table des symboles
    addSymbolToSymbolTableFromContext(memberCtx, varName, type);

    if (memberCtx->expr())
    {
      // Si une expression d'initialisation est présente, l'évalue
      shared_ptr<Symbol> symbol = getSymbolFromSymbolTableByContext(memberCtx, varName);
      shared_ptr<Symbol> source = visit(memberCtx->expr()).as<shared_ptr<Symbol>>();
      curCfg->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {symbol, source});
    }
    else
    {
      // Sinon, initialise implicitement la variable à 0
      shared_ptr<Symbol> symbol = getSymbolFromSymbolTableByContext(memberCtx, varName);
      auto zero = curCfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT, {"0"});
      curCfg->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {symbol, zero});
    }
  }
  return 0;
}

// Visite du nœud Var_assign_stmt pour assigner une valeur à une variable
antlrcpp::Any
CodeGenVisitor::visitVar_assign_stmt(ifccParser::Var_assign_stmtContext *ctx)
{
  // Récupère le symbole correspondant à l'identifiant dans la table des symboles
  shared_ptr<Symbol> symbol = getSymbolFromSymbolTableByContext(ctx, ctx->ID()->toString());

  // Si le symbole n'existe pas, retourne une erreur
  if (symbol == nullptr)
  {
    return 1;
  }

  // Évalue l'expression associée à l'assignation
  shared_ptr<Symbol> source =
      visit(ctx->expr()).as<shared_ptr<Symbol>>();

  // Récupère l'opérateur d'assignation à partir du texte brut
  std::string fullText = ctx->getText();
  size_t idLength = ctx->ID()->getText().length();
  size_t exprStart = fullText.find(ctx->expr()->getText());
  std::string op = fullText.substr(idLength, exprStart - idLength);

  // Si l'opérateur est une assignation simple
  if (op == "=")
  {
    // Ajoute une instruction IR pour assigner la valeur
    curCfg->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {symbol, source});
  }
  else
  {
    // Sinon, détermine l'opération correspondante
    IRInstr::Operation instr;
    if (op == "+=")
      instr = IRInstr::add;
    else if (op == "-=")
      instr = IRInstr::sub;
    else if (op == "*=")
      instr = IRInstr::mul;
    else if (op == "/=")
      instr = IRInstr::div;

    // Crée une variable temporaire pour stocker le résultat intermédiaire
    shared_ptr<Symbol> temp = curCfg->create_new_tempvar(Type::INT);
    curCfg->current_bb->add_IRInstr(IRInstr::ldvar, Type::INT, {symbol});
    curCfg->current_bb->add_IRInstr(instr, Type::INT, {symbol, source, temp});
    curCfg->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {symbol, temp});
  }
  return 0;
}

// Visite du nœud If pour gérer les instructions conditionnelles
antlrcpp::Any CodeGenVisitor::visitIf(ifccParser::IfContext *ctx)
{
  // Évalue l'expression conditionnelle
  shared_ptr<Symbol> result =
      visit(ctx->expr()).as<shared_ptr<Symbol>>();
  string nextBBLabel = ".L" + to_string(nextLabel);
  nextLabel++;

  // Crée les blocs de base pour la condition et les branches
  BasicBlock *baseBlock = curCfg->current_bb;
  BasicBlock *trueBlock = new BasicBlock(curCfg.get(), "");
  BasicBlock *falseBlock = new BasicBlock(curCfg.get(), nextBBLabel);

  // Ajoute une instruction IR pour comparer la condition
  baseBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result});

  // Configure les sorties des blocs
  trueBlock->exit_true = falseBlock;
  falseBlock->exit_true = baseBlock->exit_true;

  // Ajoute les blocs au CFG
  curCfg->add_bb(trueBlock);
  visit(ctx->block());

  curCfg->add_bb(falseBlock);

  baseBlock->exit_true = trueBlock;
  baseBlock->exit_false = falseBlock;
  return 0;
}

// Visite du nœud If_else pour gérer les instructions conditionnelles avec une branche else
antlrcpp::Any CodeGenVisitor::visitIf_else(ifccParser::If_elseContext *ctx)
{
  // Évalue l'expression conditionnelle
  shared_ptr<Symbol> result =
      visit(ctx->expr()).as<shared_ptr<Symbol>>();
  string elseBBLabel = ".L" + to_string(nextLabel);
  nextLabel++;
  string endBBLabel = ".L" + to_string(nextLabel);
  nextLabel++;

  // Crée les blocs de base pour la condition, les branches et la fin
  BasicBlock *baseBlock = curCfg->current_bb;
  BasicBlock *trueBlock = new BasicBlock(curCfg.get(), "");
  BasicBlock *elseBlock = new BasicBlock(curCfg.get(), elseBBLabel);
  BasicBlock *endBlock = new BasicBlock(curCfg.get(), endBBLabel);

  // Configure les sorties des blocs
  trueBlock->exit_true = endBlock;
  elseBlock->exit_true = endBlock;
  endBlock->exit_true = baseBlock->exit_true;

  // Ajoute une instruction IR pour comparer la condition
  baseBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result});

  // Ajoute les blocs au CFG et visite les blocs if et else
  curCfg->add_bb(trueBlock);
  visit(ctx->if_block);

  curCfg->add_bb(elseBlock);
  visit(ctx->else_block);

  curCfg->add_bb(endBlock);

  baseBlock->exit_true = trueBlock;
  baseBlock->exit_false = elseBlock;
  return 0;
}

// Visite du nœud While_stmt pour gérer les boucles while
antlrcpp::Any
CodeGenVisitor::visitWhile_stmt(ifccParser::While_stmtContext *ctx)
{
  // Crée les étiquettes pour les blocs de condition et de fin
  string conditionBBLabel = ".L" + to_string(nextLabel);
  nextLabel++;
  string endBBLabel = ".L" + to_string(nextLabel);
  nextLabel++;

  // Crée les blocs de base pour la condition, le corps de la boucle et la fin
  BasicBlock *baseBlock = curCfg->current_bb;
  BasicBlock *conditionBlock = new BasicBlock(curCfg.get(), conditionBBLabel);
  BasicBlock *stmtBlock = new BasicBlock(curCfg.get(), "");
  BasicBlock *endBlock = new BasicBlock(curCfg.get(), endBBLabel);

  // Configure les sorties des blocs
  conditionBlock->exit_true = stmtBlock;
  conditionBlock->exit_false = endBlock;
  endBlock->exit_true = baseBlock->exit_true;
  stmtBlock->exit_true = conditionBlock;
  baseBlock->exit_true = conditionBlock;

  // Ajoute les blocs au CFG
  curCfg->add_bb(conditionBlock);
  shared_ptr<Symbol> result =
      visit(ctx->expr()).as<shared_ptr<Symbol>>();
  conditionBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result});

  curCfg->add_bb(stmtBlock);
  visit(ctx->block());

  curCfg->add_bb(endBlock);

  return 0;
}

// Visite du nœud Block pour gérer les blocs de code
antlrcpp::Any CodeGenVisitor::visitBlock(ifccParser::BlockContext *ctx)
{
  // Pousse une nouvelle table des symboles pour le bloc
  curCfg->push_table();
  
  // Parcourt toutes les instructions du bloc
  for (ifccParser::StmtContext *stmt : ctx->stmt())
  {
    visit(stmt); // Visite chaque instruction
  }

  // Retire la table des symboles à la fin du bloc
  curCfg->pop_table();
  return 0;
}

// Visite du nœud Return_stmt pour gérer les instructions de retour
antlrcpp::Any
CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
  // Vérifie si la fonction a un type de retour non void
  if (curCfg->get_return_type() != Type::VOID)
  {
    // Si une expression de retour est absente, signale une erreur
    if (ctx->expr() == nullptr)
    {
      string message =
          "No void function " + curCfg->get_name() + " should return a value";
      VisitorErrorListener::addError(ctx, message);
      return 1;
    }
    // Évalue l'expression de retour
    shared_ptr<Symbol> val =
        visit(ctx->expr()).as<shared_ptr<Symbol>>();
    // Ajoute une instruction IR pour le retour
    curCfg->current_bb->add_IRInstr(IRInstr::ret, curCfg->get_return_type(),
                                    {val});
  }
  else
  {
    // Si la fonction est de type void, une expression de retour est interdite
    if (ctx->expr() != nullptr)
    {
      string message =
          "Void function " + curCfg->get_name() + " should not return a value";
      VisitorErrorListener::addError(ctx, message);
      return 1;
    }
    // Ajoute une instruction IR pour le retour sans valeur
    curCfg->current_bb->add_IRInstr(IRInstr::ret, curCfg->get_return_type(),
                                    {});
  }

  return 0;
}

// Visite du nœud Par pour gérer les expressions entre parenthèses
antlrcpp::Any CodeGenVisitor::visitPar(ifccParser::ParContext *ctx)
{
  // Visite l'expression contenue dans les parenthèses
  return visit(ctx->expr());
}

// Visite du nœud Func_call pour gérer les appels de fonction
antlrcpp::Any
CodeGenVisitor::visitFunc_call(ifccParser::Func_callContext *ctx)
{
  // Vérifie si la fonction appelée a été déclarée
  auto it = functions.find(ctx->ID()->toString());
  if (it == functions.end())
  {
    string message =
        "Function " + ctx->ID()->toString() + " was not declared";
    VisitorErrorListener::addError(ctx, message);
  }

  auto funcCfg = it->second;

  // Vérifie si le nombre de paramètres correspond
  if (ctx->expr().size() != funcCfg->get_parameters_type().size())
  {
    string message = "Wrong number of parameters in function call to " +
                     funcCfg->get_name() + ": expected " +
                     to_string(funcCfg->get_parameters_type().size()) +
                     " but found " + to_string(ctx->expr().size()) +
                     " instead";
    VisitorErrorListener::addError(ctx, message);
  }

  // Prépare les paramètres pour l'appel de fonction
  vector<Parameter> params = {ctx->ID()->toString()};
  for (int i = 0; i < funcCfg->get_parameters_type().size(); i++)
  {
    shared_ptr<Symbol> symbol =
        visit(ctx->expr(i)).as<shared_ptr<Symbol>>();
    params.push_back(symbol);
    // Ajoute une instruction IR pour chaque paramètre
    curCfg->current_bb->add_IRInstr(
        IRInstr::param, funcCfg->get_parameters_type()[i].type, {symbol});
  }

  // Ajoute une instruction IR pour l'appel de fonction
  return curCfg->current_bb->add_IRInstr(IRInstr::call,
                                         funcCfg->get_return_type(), params);
}

// Visite du nœud Multdiv pour gérer les opérations de multiplication, division et modulo
antlrcpp::Any CodeGenVisitor::visitMultdiv(ifccParser::MultdivContext *ctx)
{
  IRInstr::Operation instr;
  // Détermine l'opération en fonction de l'opérateur
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

  // Évalue les opérandes gauche et droit
  shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<shared_ptr<Symbol>>();

  // Ajoute une instruction IR pour l'opération
  return curCfg->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
}

// Visite du nœud Addsub pour gérer les opérations d'addition et de soustraction
antlrcpp::Any CodeGenVisitor::visitAddsub(ifccParser::AddsubContext *ctx)
{
  // Détermine l'opération en fonction de l'opérateur (+ ou -)
  IRInstr::Operation instr =
      (ctx->op->getText() == "+" ? IRInstr::add : IRInstr::sub);

  // Évalue les opérandes gauche et droit
  shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<shared_ptr<Symbol>>();

  // Vérifie si les opérandes sont valides
  if (leftVal == nullptr || rightVal == nullptr)
  {
    VisitorErrorListener::addError(
        ctx, "Invalid operation with function returning void");
  }

  // Ajoute une instruction IR pour l'opération
  return curCfg->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
}

// Visite du nœud Cmp pour gérer les comparaisons (<, <=, >, >=)
antlrcpp::Any CodeGenVisitor::visitCmp(ifccParser::CmpContext *ctx)
{
  IRInstr::Operation instr;
  // Détermine l'opération en fonction de l'opérateur
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

  // Évalue les opérandes gauche et droit
  shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<shared_ptr<Symbol>>();

  // Vérifie si les opérandes sont valides
  if (leftVal == nullptr || rightVal == nullptr)
  {
    VisitorErrorListener::addError(
        ctx, "Invalid operation with function returning void");
  }

  // Ajoute une instruction IR pour la comparaison
  return curCfg->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
}

// Visite du nœud Eq pour gérer les comparaisons d'égalité (==, !=)
antlrcpp::Any CodeGenVisitor::visitEq(ifccParser::EqContext *ctx)
{
  // Détermine l'opération en fonction de l'opérateur (== ou !=)
  IRInstr::Operation instr =
      (ctx->op->getText() == "==" ? IRInstr::eq : IRInstr::neq);

  // Évalue les opérandes gauche et droit
  shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<shared_ptr<Symbol>>();

  // Vérifie si les opérandes sont valides
  if (leftVal == nullptr || rightVal == nullptr)
  {
    VisitorErrorListener::addError(
        ctx, "Invalid operation with function returning void");
  }

  // Ajoute une instruction IR pour la comparaison
  return curCfg->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
}

// Visite du nœud Val pour gérer les valeurs (identifiants, littéraux entiers ou caractères)
antlrcpp::Any CodeGenVisitor::visitVal(ifccParser::ValContext *ctx)
{
  shared_ptr<Symbol> source;
  // Si c'est un identifiant, charge la variable
  if (ctx->ID() != nullptr)
  {
    shared_ptr<Symbol> symbol = getSymbolFromSymbolTableByContext(ctx, ctx->ID()->toString());
    if (symbol != nullptr)
    {
      source =
          curCfg->current_bb->add_IRInstr(IRInstr::ldvar, Type::INT, {symbol});
    }
  }
  // Si c'est un littéral entier, charge la constante
  else if (ctx->INTEGER_LITERAL() != nullptr)
  {
    source = curCfg->current_bb->add_IRInstr(
        IRInstr::ldconst, Type::INT, {ctx->INTEGER_LITERAL()->toString()});
  }
  // Si c'est un littéral caractère, charge la constante
  else if (ctx->CHAR_LITERAL() != nullptr)
  {
    string val =
        to_string(static_cast<int>(ctx->CHAR_LITERAL()->toString()[1]));
    source =
        curCfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::CHAR, {val});
  }

  return source;
}

// Ajoute un symbole à la table des symboles à partir du contexte
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

// Récupère un symbole de la table des symboles à partir du contexte
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

// Visite du nœud B_and pour gérer les opérations binaires AND (&)
antlrcpp::Any CodeGenVisitor::visitB_and(ifccParser::B_andContext *ctx)
{
  // Évalue les opérandes gauche et droit
  shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<shared_ptr<Symbol>>();

  // Ajoute une instruction IR pour l'opération AND
  return curCfg->current_bb->add_IRInstr(IRInstr::b_and, Type::INT,
                                         {leftVal, rightVal});
}

// Visite du nœud B_or pour gérer les opérations binaires OR (|)
antlrcpp::Any CodeGenVisitor::visitB_or(ifccParser::B_orContext *ctx)
{
  // Évalue les opérandes gauche et droit
  shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<shared_ptr<Symbol>>();

  // Vérifie si les opérandes sont valides
  if (leftVal == nullptr || rightVal == nullptr)
  {
    VisitorErrorListener::addError(
        ctx, "Invalid operation with function returning void");
  }

  // Ajoute une instruction IR pour l'opération OR
  return curCfg->current_bb->add_IRInstr(IRInstr::b_or, Type::INT,
                                         {leftVal, rightVal});
}

// Visite du nœud B_xor pour gérer les opérations binaires XOR (^)
antlrcpp::Any CodeGenVisitor::visitB_xor(ifccParser::B_xorContext *ctx)
{
  // Évalue les opérandes gauche et droit
  shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<shared_ptr<Symbol>>();

  // Vérifie si les opérandes sont valides
  if (leftVal == nullptr || rightVal == nullptr)
  {
    VisitorErrorListener::addError(
        ctx, "Invalid operation with function returning void");
  }

  // Ajoute une instruction IR pour l'opération XOR
  return curCfg->current_bb->add_IRInstr(IRInstr::b_xor, Type::INT,
                                         {leftVal, rightVal});
}

// Visite du nœud UnaryOp pour gérer les opérations unaires (-, ~, !, ++, --, +)
antlrcpp::Any CodeGenVisitor::visitUnaryOp(ifccParser::UnaryOpContext *ctx)
{
  // Évalue l'opérande
  shared_ptr<Symbol> val =
      visit(ctx->expr()).as<shared_ptr<Symbol>>();
  IRInstr::Operation instr;

  // Détermine l'opération en fonction de l'opérateur unaire
  if (ctx->op->getText() == "-")
  {
    // Négation arithmétique
    instr = IRInstr::neg;
    return curCfg->current_bb->add_IRInstr(instr, Type::INT, {val});
  }
  else if (ctx->op->getText() == "~")
  {
    // Négation binaire
    instr = IRInstr::not_;
    return curCfg->current_bb->add_IRInstr(instr, Type::INT, {val});
  }
  else if (ctx->op->getText() == "!")
  {
    // Négation logique
    instr = IRInstr::lnot;
    return curCfg->current_bb->add_IRInstr(instr, Type::INT, {val});
  }
  else if (ctx->op->getText() == "++")
  {
    // Incrémentation
    instr = IRInstr::inc;
    return curCfg->current_bb->add_IRInstr(instr, Type::INT, {val});
  }
  else if (ctx->op->getText() == "--")
  {
    // Décrémentation
    instr = IRInstr::dec;
    return curCfg->current_bb->add_IRInstr(instr, Type::INT, {val});
  }
  else if (ctx->op->getText() == "+")
  {
    // Opérateur unaire + (aucune opération nécessaire)
    return val;
  }

  // Retourne nullptr si l'opérateur n'est pas reconnu
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