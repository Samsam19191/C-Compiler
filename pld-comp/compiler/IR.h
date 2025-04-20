#pragma once

// Inclusions nécessaires
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stack>
#include <string>
#include <variant>
#include <vector>

#include "Symbol.h"
#include "Type.h"
#include "ErrorListenerVisitor.h"

using namespace std;

// Déclarations anticipées
class BasicBlock;
class CFG;
class CodeGenVisitor;

// Alias pour une table des symboles
typedef map<string, shared_ptr<Symbol>> SymbolTable;

// Un paramètre peut être soit un symbole (variable), soit une chaîne littérale (ex: label)
typedef variant<shared_ptr<Symbol>, string> Parameter;

// Registres utilisés pour la génération de code assembleur
const string registers8[] = {"r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"};
const string registers32[] = {"r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"};
const string registers64[] = {"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"};
const string paramRegisters[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};

// Surcharge pour l'affichage d'un paramètre (utile pour debug ou génération)
ostream &operator<<(ostream &os, const Parameter &param);

// ========== Classe IRInstr ==========
// Représente une instruction intermédiaire (IR) dans un basic block
class IRInstr
{
public:
  // Liste des types d'opérations IR supportées
  typedef enum
  {
    var_assign,
    ldconst,
    ldvar,
    add,
    sub,
    mul,
    div,
    mod,
    b_and,
    b_or,
    b_xor,
    cmpNZ,
    ret,
    leq,
    lt,
    geq,
    gt,
    eq,
    neq,
    neg,
    not_,
    lnot,
    inc,
    dec,
    nothing,
    call,
    param,
    param_decl
  } Operation;

  // Constructeur
  IRInstr(BasicBlock *basicBlock, Operation operation, Type type, const vector<Parameter> &parameters);

  // Génère le code assembleur pour cette instruction
  void genAsm(ostream &os, CFG *cfg);

  friend ostream &operator<<(ostream &os, IRInstr &instruction);

  // Fonctions utilitaires pour l'allocation de registres
  set<shared_ptr<Symbol>> getUsedVariables();    // Retourne les variables utilisées
  set<shared_ptr<Symbol>> getDeclaredVariable(); // Retourne celles déclarées ici

private:
  Type outType;                  // Type de retour
  vector<Parameter> parameters; // Paramètres de l'instruction
  Operation operation;                  // Type de l'instruction
  BasicBlock *block;             // Basic block auquel cette instruction appartient

  // Fonctions de génération d'assembleur pour les différents types d'opérations
  void generateCompareNotZero(ostream &os, CFG *cfg);
  void generateDivisionInstruction(ostream &os, CFG *cfg);
  void generateModuloInstruction(ostream &os, CFG *cfg);
  void generateReturnInstruction(ostream &os, CFG *cfg);
  void generateVariableAssignment(ostream &os, CFG *cfg);
  void generateLoadConstant(ostream &os, CFG *cfg);
  void generateLoadVariable(ostream &os, CFG *cfg);
  void generateUnaryOperation(const string &operation, ostream &os, CFG *cfg);
  void generateFunctionCall(ostream &os, CFG *cfg);
  void generateFunctionParameterPassing(ostream &os, CFG *cfg);
  void generateBinaryOperation(const string &operation, ostream &os, CFG *cfg);
  void generateComparisonOperation(const string &operation, ostream &os, CFG *cfg);
};