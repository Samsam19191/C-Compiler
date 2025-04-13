#pragma once

#include "ParserRuleContext.h"
#include "Symbol.h"
#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "IR.h"
#include <map>
#include <memory>

using namespace std;

/**
 * @brief Classe de visiteur utilisée pour parcourir l'AST généré par ANTLR
 *        et produire le code intermédiaire (IR).
 *
 * Hérite de ifccBaseVisitor (généré automatiquement par ANTLR à partir de la grammaire).
 */
class CodeGenVisitor : public ifccBaseVisitor {
public:
  // Destructeur par défaut
  virtual ~CodeGenVisitor() = default;

    // Constructeur
  CodeGenVisitor();

    // Entrée principale du programme
  virtual antlrcpp::Any visitAxiom(ifccParser::AxiomContext *ctx) override;
  
    // Racine du programme (plusieurs fonctions possibles)

  virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
  // Visite d'une fonction complète

  virtual antlrcpp::Any visitFunc(ifccParser::FuncContext *ctx) override;
  // Instruction "return"

  virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;
  // Déclaration de variable (ex : int x;)

  virtual antlrcpp::Any visitVar_decl_stmt(ifccParser::Var_decl_stmtContext *ctx) override;
  // Affectation de variable (ex : x = 5;)

  virtual antlrcpp::Any visitVar_assign_stmt(ifccParser::Var_assign_stmtContext *ctx) override;
  // Expression entre parenthèses

  virtual antlrcpp::Any visitPar(ifccParser::ParContext *ctx) override;
  // Condition "if" sans "else"

  virtual antlrcpp::Any visitIf(ifccParser::IfContext *ctx) override;
  // Condition "if ... else"

  virtual antlrcpp::Any visitIf_else(ifccParser::If_elseContext *ctx) override;
  // Boucle "while"

  virtual antlrcpp::Any visitWhile_stmt(ifccParser::While_stmtContext *ctx) override;
  // Bloc de code (ex : { ... })

  virtual antlrcpp::Any visitBlock(ifccParser::BlockContext *ctx) override;
  // Appel de fonction

  virtual antlrcpp::Any visitFunc_call(ifccParser::Func_callContext *ctx) override;
  // Opérations * et / (multiplication/division)

  virtual antlrcpp::Any visitMultdiv(ifccParser::MultdivContext *ctx) override;
  // Opérations + et - (addition/soustraction)

  virtual antlrcpp::Any visitAddsub(ifccParser::AddsubContext *ctx) override;
  // Comparateurs relationnels (ex : <, >, <=, >=)

  virtual antlrcpp::Any visitCmp(ifccParser::CmpContext *ctx) override;
  // Comparateurs d'égalité (==, !=)

  virtual antlrcpp::Any visitEq(ifccParser::EqContext *ctx) override;
  // Valeur littérale ou identifiant

  virtual antlrcpp::Any visitVal(ifccParser::ValContext *ctx) override;
  // Opération bitwise AND ( & )

  virtual antlrcpp::Any visitB_and(ifccParser::B_andContext *ctx) override;
  // Opération bitwise OR ( | )

  virtual antlrcpp::Any visitB_or(ifccParser::B_orContext *ctx) override;
  // Opération bitwise XOR ( ^ )

  virtual antlrcpp::Any visitB_xor(ifccParser::B_xorContext *ctx) override;
  // Opérateurs unaires (ex : -x, !x, ~x)

  virtual antlrcpp::Any visitUnaryOp(ifccParser::UnaryOpContext *ctx) override;
  
  virtual antlrcpp::Any visitPostIncDec(ifccParser::PostIncDecContext *ctx) override;

  virtual antlrcpp::Any visitPreIncDec(ifccParser::PreIncDecContext *ctx) override;

  virtual antlrcpp::Any visitLogicalOr(ifccParser::LogicalOrContext *ctx) override;

  virtual antlrcpp::Any visitLogicalAnd(ifccParser::LogicalAndContext *ctx) override;
  /**
   * @brief Accès à la liste des CFG (Control Flow Graphs) générés pour chaque fonction
   */
  const vector<shared_ptr<CFG>> &getCfgList() const {
    return cfgList;
  }

  /**
   * @brief Récupère le CFG correspondant à une fonction par son nom
   */
  CFG *getFunction(const string &id) { return functions[id].get(); }

private:
  // Numéro utilisé pour générer des labels uniques dans l'IR (pour les sauts)
  int nextLabel = 1;

    // Liste des CFG générés pour chaque fonction rencontrée

  vector<shared_ptr<CFG>> cfgList;
    // Map des fonctions par nom, associées à leur CFG

  map<string, shared_ptr<CFG>> functions;
    // CFG courant, modifié à chaque nouvelle fonction rencontrée

  shared_ptr<CFG> curCfg;
  // Buffer interne pour stocker du code assembleur éventuel

  stringstream assembly;

 /**
   * @brief Ajoute un symbole (variable, paramètre...) à la table des symboles du CFG courant,
   *        en vérifiant s'il n'est pas déjà déclaré.
   *
   * @param ctx Le contexte ANTLR correspondant
   * @param id  Le nom de l'identifiant à ajouter
   * @param type Le type de la variable (int, char, etc.)
   */
  bool addSymbolToSymbolTableFromContext(antlr4::ParserRuleContext *ctx, const string &id,
    Type type);

/**
* @brief Récupère un symbole depuis la table des symboles du CFG courant,
*        avec vérification du contexte.
*
* @param ctx Le contexte ANTLR où apparaît le symbole
* @param id  Le nom de l'identifiant à récupérer
*/
shared_ptr<Symbol> getSymbolFromSymbolTableByContext(antlr4::ParserRuleContext *ctx,
                       const string &id);
};