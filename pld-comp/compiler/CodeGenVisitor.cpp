#include "CodeGenVisitor.h"
#include "Type.h"
#include "ErrorListenerVisitor.h"
#include "IR.h"
#include "CFG.h"
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
  auto symbole = putchar->add_parameter("c", Type::INT, 0);
  symbole->used = true; // Marque le symbole comme utilisé

  // Ajout des fonctions getchar et putchar à la liste des CFG et au dictionnaire des fonctions
  functionCFGs.push_back(getchar);
  functions["getchar"] = getchar;
  functionCFGs.push_back(putchar);
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
    currentCFG =
        make_shared<CFG>(type, func->ID(0)->toString(), argCount, this);

    // Ajoute la CFG à la liste et au dictionnaire des fonctions
    functionCFGs.push_back(currentCFG);
    functions[func->ID(0)->toString()] = currentCFG;

    // Visite le contenu de la fonction
    visit(func);

    // Nettoie la table des symboles
    currentCFG->pop_table();
  }

  // Si des erreurs ont été détectées, termine le programme
  if (ErrorListenerVisitor::hasError())
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
    auto symbole = currentCFG->add_parameter(ctx->ID(i)->toString(), type,
                                        ctx->getStart()->getLine());
    currentCFG->current_bb->add_IRInstr(IRInstr::param_decl, type, {symbole});
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
    ErrorListenerVisitor::addError(ctx, "Can't create a variable of type void");
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
      shared_ptr<Symbol> symbole = getSymbolFromSymbolTableByContext(memberCtx, varName);
      shared_ptr<Symbol> source = visit(memberCtx->expr()).as<shared_ptr<Symbol>>();
      currentCFG->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {symbole, source});
    }
    else
    {
      // Sinon, initialise implicitement la variable à 0
      shared_ptr<Symbol> symbole = getSymbolFromSymbolTableByContext(memberCtx, varName);
      auto zero = currentCFG->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT, {"0"});
      currentCFG->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {symbole, zero});
    }
  }
  return 0;
}

// Visite du nœud Var_assign_stmt pour assigner une valeur à une variable
antlrcpp::Any
CodeGenVisitor::visitVar_assign_stmt(ifccParser::Var_assign_stmtContext *ctx)
{
  // Récupère le symbole correspondant à l'identifiant dans la table des symboles
  shared_ptr<Symbol> symbole = getSymbolFromSymbolTableByContext(ctx, ctx->ID()->toString());

  // Si le symbole n'existe pas, retourne une erreur
  if (symbole == nullptr)
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
    currentCFG->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {symbole, source});
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
    shared_ptr<Symbol> temp = currentCFG->create_new_tempvar(Type::INT);
    currentCFG->current_bb->add_IRInstr(IRInstr::ldvar, Type::INT, {symbole});
    currentCFG->current_bb->add_IRInstr(instr, Type::INT, {symbole, source, temp});
    currentCFG->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {symbole, temp});
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
  BasicBlock *baseBlock = currentCFG->current_bb;
  BasicBlock *trueBlock = new BasicBlock(currentCFG.get(), "");
  BasicBlock *falseBlock = new BasicBlock(currentCFG.get(), nextBBLabel);

  // Ajoute une instruction IR pour comparer la condition
  baseBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result});

  // Configure les sorties des blocs
  trueBlock->exit_true = falseBlock;
  falseBlock->exit_true = baseBlock->exit_true;

  // Ajoute les blocs au CFG
  currentCFG->add_bb(trueBlock);
  visit(ctx->block());

  currentCFG->add_bb(falseBlock);

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
  BasicBlock *baseBlock = currentCFG->current_bb;
  BasicBlock *trueBlock = new BasicBlock(currentCFG.get(), "");
  BasicBlock *elseBlock = new BasicBlock(currentCFG.get(), elseBBLabel);
  BasicBlock *endBlock = new BasicBlock(currentCFG.get(), endBBLabel);

  // Configure les sorties des blocs
  trueBlock->exit_true = endBlock;
  elseBlock->exit_true = endBlock;
  endBlock->exit_true = baseBlock->exit_true;

  // Ajoute une instruction IR pour comparer la condition
  baseBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result});

  // Ajoute les blocs au CFG et visite les blocs if et else
  currentCFG->add_bb(trueBlock);
  visit(ctx->if_block);

  currentCFG->add_bb(elseBlock);
  visit(ctx->else_block);

  currentCFG->add_bb(endBlock);

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
  BasicBlock *baseBlock = currentCFG->current_bb;
  BasicBlock *conditionBlock = new BasicBlock(currentCFG.get(), conditionBBLabel);
  BasicBlock *stmtBlock = new BasicBlock(currentCFG.get(), "");
  BasicBlock *endBlock = new BasicBlock(currentCFG.get(), endBBLabel);

  // Configure les sorties des blocs
  conditionBlock->exit_true = stmtBlock;
  conditionBlock->exit_false = endBlock;
  endBlock->exit_true = baseBlock->exit_true;
  stmtBlock->exit_true = conditionBlock;
  baseBlock->exit_true = conditionBlock;

  // Ajoute les blocs au CFG
  currentCFG->add_bb(conditionBlock);
  shared_ptr<Symbol> result =
      visit(ctx->expr()).as<shared_ptr<Symbol>>();
  conditionBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result});

  currentCFG->add_bb(stmtBlock);
  visit(ctx->block());

  currentCFG->add_bb(endBlock);

  return 0;
}

// Visite du nœud Block pour gérer les blocs de code
antlrcpp::Any CodeGenVisitor::visitBlock(ifccParser::BlockContext *ctx)
{
  // Pousse une nouvelle table des symboles pour le bloc
  currentCFG->push_table();
  
  // Parcourt toutes les instructions du bloc
  for (ifccParser::StmtContext *stmt : ctx->stmt())
  {
    visit(stmt); // Visite chaque instruction
  }

  // Retire la table des symboles à la fin du bloc
  currentCFG->pop_table();
  return 0;
}

// Visite du nœud Return_stmt pour gérer les instructions de retour
antlrcpp::Any
CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
  // Vérifie si la fonction a un type de retour non void
  if (currentCFG->get_return_type() != Type::VOID)
  {
    // Si une expression de retour est absente, signale une erreur
    if (ctx->expr() == nullptr)
    {
      string message =
          "No void function " + currentCFG->get_name() + " should return a value";
      ErrorListenerVisitor::addError(ctx, message);
      return 1;
    }
    // Évalue l'expression de retour
    shared_ptr<Symbol> val =
        visit(ctx->expr()).as<shared_ptr<Symbol>>();
    // Ajoute une instruction IR pour le retour
    currentCFG->current_bb->add_IRInstr(IRInstr::ret, currentCFG->get_return_type(),
                                    {val});
  }
  else
  {
    // Si la fonction est de type void, une expression de retour est interdite
    if (ctx->expr() != nullptr)
    {
      string message =
          "Void function " + currentCFG->get_name() + " should not return a value";
      ErrorListenerVisitor::addError(ctx, message);
      return 1;
    }
    // Ajoute une instruction IR pour le retour sans valeur
    currentCFG->current_bb->add_IRInstr(IRInstr::ret, currentCFG->get_return_type(),
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
    ErrorListenerVisitor::addError(ctx, message);
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
    ErrorListenerVisitor::addError(ctx, message);
  }

  // Prépare les paramètres pour l'appel de fonction
  vector<Parameter> params = {ctx->ID()->toString()};
  for (int i = 0; i < funcCfg->get_parameters_type().size(); i++)
  {
    shared_ptr<Symbol> symbole =
        visit(ctx->expr(i)).as<shared_ptr<Symbol>>();
    params.push_back(symbole);
    // Ajoute une instruction IR pour chaque paramètre
    currentCFG->current_bb->add_IRInstr(
        IRInstr::param, funcCfg->get_parameters_type()[i].type, {symbole});
  }

  // Ajoute une instruction IR pour l'appel de fonction
  return currentCFG->current_bb->add_IRInstr(IRInstr::call,
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
  return currentCFG->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
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
    ErrorListenerVisitor::addError(
        ctx, "Invalid operation with function returning void");
  }

  // Ajoute une instruction IR pour l'opération
  return currentCFG->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
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
    ErrorListenerVisitor::addError(
        ctx, "Invalid operation with function returning void");
  }

  // Ajoute une instruction IR pour la comparaison
  return currentCFG->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
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
    ErrorListenerVisitor::addError(
        ctx, "Invalid operation with function returning void");
  }

  // Ajoute une instruction IR pour la comparaison
  return currentCFG->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
}

// Visite du nœud Val pour gérer les valeurs (identifiants, littéraux entiers ou caractères)
antlrcpp::Any CodeGenVisitor::visitVal(ifccParser::ValContext *ctx)
{
  shared_ptr<Symbol> source;
  // Si c'est un identifiant, charge la variable
  if (ctx->ID() != nullptr)
  {
    shared_ptr<Symbol> symbole = getSymbolFromSymbolTableByContext(ctx, ctx->ID()->toString());
    if (symbole != nullptr)
    {
      source =
          currentCFG->current_bb->add_IRInstr(IRInstr::ldvar, Type::INT, {symbole});
    }
  }
  // Si c'est un littéral entier, charge la constante
  else if (ctx->INTEGER_LITERAL() != nullptr)
  {
    source = currentCFG->current_bb->add_IRInstr(
        IRInstr::ldconst, Type::INT, {ctx->INTEGER_LITERAL()->toString()});
  }
  // Si c'est un littéral caractère, charge la constante
  else if (ctx->CHAR_LITERAL() != nullptr)
  {
    string val =
        to_string(static_cast<int>(ctx->CHAR_LITERAL()->toString()[1]));
    source =
        currentCFG->current_bb->add_IRInstr(IRInstr::ldconst, Type::CHAR, {val});
  }

  return source;
}

// Ajoute un symbole à la table des symboles à partir du contexte
bool CodeGenVisitor::addSymbolToSymbolTableFromContext(antlr4::ParserRuleContext *ctx,
                                                       const string &id, Type type)
{
  bool result = currentCFG->add_symbol(id, type, ctx->getStart()->getLine());
  if (!result)
  {
    string error = "The variable " + id + " has already been declared";
    ErrorListenerVisitor::addError(ctx, error, ErrorType::Error);
  }
  return result;
}

// Récupère un symbole de la table des symboles à partir du contexte
shared_ptr<Symbol>
CodeGenVisitor::getSymbolFromSymbolTableByContext(antlr4::ParserRuleContext *ctx,
                                                  const string &id)
{
  shared_ptr<Symbol> symbole = currentCFG->get_symbol(id);
  if (symbole == nullptr)
  {
    const string error = "Symbol not found: " + id;
    ErrorListenerVisitor::addError(ctx, error, ErrorType::Error);
    return nullptr;
  }

  symbole->used = true;
  return symbole;
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
  return currentCFG->current_bb->add_IRInstr(IRInstr::b_and, Type::INT,
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
    ErrorListenerVisitor::addError(
        ctx, "Invalid operation with function returning void");
  }

  // Ajoute une instruction IR pour l'opération OR
  return currentCFG->current_bb->add_IRInstr(IRInstr::b_or, Type::INT,
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
    ErrorListenerVisitor::addError(
        ctx, "Invalid operation with function returning void");
  }

  // Ajoute une instruction IR pour l'opération XOR
  return currentCFG->current_bb->add_IRInstr(IRInstr::b_xor, Type::INT,
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
    return currentCFG->current_bb->add_IRInstr(instr, Type::INT, {val});
  }
  else if (ctx->op->getText() == "~")
  {
    // Négation binaire
    instr = IRInstr::not_;
    return currentCFG->current_bb->add_IRInstr(instr, Type::INT, {val});
  }
  else if (ctx->op->getText() == "!")
  {
    // Négation logique
    instr = IRInstr::lnot;
    return currentCFG->current_bb->add_IRInstr(instr, Type::INT, {val});
  }
  else if (ctx->op->getText() == "++")
  {
    // Incrémentation
    instr = IRInstr::inc;
    return currentCFG->current_bb->add_IRInstr(instr, Type::INT, {val});
  }
  else if (ctx->op->getText() == "--")
  {
    // Décrémentation
    instr = IRInstr::dec;
    return currentCFG->current_bb->add_IRInstr(instr, Type::INT, {val});
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
  shared_ptr<Symbol> symbole = getSymbolFromSymbolTableByContext(ctx, ctx->ID()->getText());

  // Récupérer l'opérateur à partir du texte brut
  std::string fullText = ctx->getText();
  std::string op = fullText.substr(fullText.size() - 2); // Les deux derniers caractères

  IRInstr::Operation operation = (op == "++") ? IRInstr::inc : IRInstr::dec;

  // Sauvegarde de la valeur avant incrémentation
  shared_ptr<Symbol> temp = currentCFG->create_new_tempvar(Type::INT);
  currentCFG->current_bb->add_IRInstr(IRInstr::ldvar, Type::INT, {symbole});
  currentCFG->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {temp, symbole});

  // Incrémentation
  currentCFG->current_bb->add_IRInstr(operation, Type::INT, {symbole});

  return temp;
}

antlrcpp::Any CodeGenVisitor::visitPreIncDec(ifccParser::PreIncDecContext *ctx)
{
  shared_ptr<Symbol> symbole = getSymbolFromSymbolTableByContext(ctx, ctx->ID()->getText());

  // Récupérer l'opérateur à partir du texte brut
  std::string fullText = ctx->getText();
  std::string op = fullText.substr(0, 2); // Les deux premiers caractères

  IRInstr::Operation operation = (op == "++") ? IRInstr::inc : IRInstr::dec;

  // Incrémentation avant utilisation
  currentCFG->current_bb->add_IRInstr(operation, Type::INT, {symbole});

  return symbole;
}

antlrcpp::Any CodeGenVisitor::visitLogicalOr(ifccParser::LogicalOrContext *ctx) {
  string falseLabel = ".L" + to_string(nextLabel++);
  string endLabel = ".L" + to_string(nextLabel++);
  
  shared_ptr<Symbol> left = visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> result = currentCFG->create_new_tempvar(Type::INT);
  
  // Évaluation paresseuse - si gauche est vrai, résultat est vrai
  currentCFG->current_bb->add_IRInstr(IRInstr::cmpNZ, Type::INT, {left});
  currentCFG->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT, {"1"});
  currentCFG->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {result, left});
  
  BasicBlock* falseBB = new BasicBlock(currentCFG.get(), falseLabel);
  BasicBlock* endBB = new BasicBlock(currentCFG.get(), endLabel);
  
  currentCFG->current_bb->exit_false = falseBB;
  currentCFG->current_bb->exit_true = endBB;
  
  currentCFG->add_bb(falseBB);
  shared_ptr<Symbol> right = visit(ctx->expr(1)).as<shared_ptr<Symbol>>();
  currentCFG->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {result, right});
  
  currentCFG->add_bb(endBB);
  return result;
}

antlrcpp::Any CodeGenVisitor::visitLogicalAnd(ifccParser::LogicalAndContext *ctx) {
  string trueLabel = ".L" + to_string(nextLabel++);
  string endLabel = ".L" + to_string(nextLabel++);
  
  shared_ptr<Symbol> left = visit(ctx->expr(0)).as<shared_ptr<Symbol>>();
  shared_ptr<Symbol> result = currentCFG->create_new_tempvar(Type::INT);
  
  // Évaluation paresseuse - si gauche est faux, résultat est faux
  currentCFG->current_bb->add_IRInstr(IRInstr::cmpNZ, Type::INT, {left});
  currentCFG->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT, {"0"});
  currentCFG->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {result, left});
  
  BasicBlock* trueBB = new BasicBlock(currentCFG.get(), trueLabel);
  BasicBlock* endBB = new BasicBlock(currentCFG.get(), endLabel);
  
  currentCFG->current_bb->exit_true = trueBB;
  currentCFG->current_bb->exit_false = endBB;
  
  currentCFG->add_bb(trueBB);
  shared_ptr<Symbol> right = visit(ctx->expr(1)).as<shared_ptr<Symbol>>();
  currentCFG->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {result, right});
  
  currentCFG->add_bb(endBB);
  return result;
}