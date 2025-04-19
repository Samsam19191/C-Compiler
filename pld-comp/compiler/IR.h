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
  IRInstr(BasicBlock *bb_, Operation op, Type t, const vector<Parameter> &parameters);

  // Génère le code assembleur pour cette instruction
  void genAsm(ostream &os, CFG *cfg);

  friend ostream &operator<<(ostream &os, IRInstr &instruction);

  // Fonctions utilitaires pour l'allocation de registres
  set<shared_ptr<Symbol>> getUsedVariables();    // Retourne les variables utilisées
  set<shared_ptr<Symbol>> getDeclaredVariable(); // Retourne celles déclarées ici

private:
  Type outType;                  // Type de retour
  vector<Parameter> parameters; // Paramètres de l'instruction
  Operation op;                  // Type de l'instruction
  BasicBlock *block;             // Basic block auquel cette instruction appartient

  // Fonctions de génération d'assembleur pour les différents types d'opérations
  void generateCompareNotZero(ostream &os, CFG *cfg);
  void generateDivisionInstruction(ostream &os, CFG *cfg);
  void generateModuloInstruction(ostream &os, CFG *cfg);
  void generateReturnInstruction(ostream &os, CFG *cfg);
  void generateVariableAssignment(ostream &os, CFG *cfg);
  void generateLoadConstant(ostream &os, CFG *cfg);
  void generateLoadVariable(ostream &os, CFG *cfg);
  void generateUnaryOperation(const string &op, ostream &os, CFG *cfg);
  void generateFunctionCall(ostream &os, CFG *cfg);
  void generateFunctionParameterPassing(ostream &os, CFG *cfg);
  void generateBinaryOperation(const string &op, ostream &os, CFG *cfg);
  void generateComparisonOperation(const string &op, ostream &os, CFG *cfg);
};

// ========== Classe BasicBlock ==========
// Représente un bloc de base dans le CFG
class BasicBlock
{
public:
  BasicBlock(CFG *cfg, string entry_label);
  void gen_asm(ostream &o); // Génère l'assembleur du bloc

  shared_ptr<Symbol> add_IRInstr(IRInstr::Operation op, Type t,
                                      vector<Parameter> parameters);

  BasicBlock *exit_true;       // Bloc suivant si condition vraie
  BasicBlock *exit_false;      // Bloc suivant si condition fausse (sinon jump inconditionnel)
  bool visited;                // Indique si ce bloc a déjà été généré (utile pour éviter les doublons)
  string label;           // Label du bloc (nom unique)
  CFG *cfg;                    // CFG auquel appartient ce bloc
  vector<IRInstr> instrs; // Liste des instructions IR dans ce bloc
  string test_var_name;   // Nom de la variable de test (pour if / while, etc.)
};

// ========== Structures auxiliaires ==========

// Représente un paramètre de fonction avec son type et symbole
struct FunctionParameter
{
  Type type;
  shared_ptr<Symbol> symbol;
  FunctionParameter(Type type, shared_ptr<Symbol> symbol)
      : type(type), symbol(symbol) {}
};

// Contient les ensembles de variables vivantes avant et après chaque instruction (analyse de vivacité)
struct InstructionLivenessAnalysis
{
  map<IRInstr *, set<shared_ptr<Symbol>>> liveVariablesBeforeInstruction;
  map<IRInstr *, set<shared_ptr<Symbol>>> liveVariablesAfterInstruction;
};

// Informations utilisées lors de l'allocation de registres : ordre de traitement et variables spillées
struct RegisterAllocationSpillData
{
  stack<shared_ptr<Symbol>> registerAssignmentOrder;
  set<shared_ptr<Symbol>> variablesThatWereSpilled;
};

// ========== Classe CFG ==========
// Représente le Control Flow Graph d'une fonction
class CFG
{
public:
  ~CFG();
  CFG(Type type, const string &name, int argCount, CodeGenVisitor *visitor);

  void add_bb(BasicBlock *bb); // Ajoute un bloc
  inline vector<BasicBlock *> &getBlocks() { return bbs; };

  string IR_reg_to_asm(string reg); // Conversion d’un registre IR en emplacement mémoire
  void gen_asm_prologue(ostream &o);     // Génère le prologue assembleur
  void gen_asm(ostream &o);              // Génère le corps
  void gen_asm_epilogue(ostream &o);     // Génère l’épilogue

  // Fonctions d'aide pour la gestion des symboles
  shared_ptr<Symbol> create_new_tempvar(Type t);
  int get_var_index(string name);
  Type get_var_type(string name);

  string new_BB_name(); // Génère un nom unique pour un nouveau bloc

  BasicBlock *current_bb;               // Bloc courant
  static const int scratchRegister = 7; // Registre temporaire

  // Gestion de la pile de tables des symboles
  inline void push_table() { symbolTables.push_front(SymbolTable()); }
  void pop_table();

  bool add_symbol(string id, Type t, int line);           // Ajoute une variable
  shared_ptr<Symbol> get_symbol(const string &name); // Récupère une variable

  string &get_name() { return name; }
  Type get_return_type() { return returnType; }
  const vector<FunctionParameter> &get_parameters_type() { return parameterTypes; }

  // Ajout et gestion des paramètres
  shared_ptr<Symbol> add_parameter(const string &name, Type type, int line);
  map<shared_ptr<Symbol>, int> registerAssignment;

  inline void push_parameter(shared_ptr<Symbol> symbol)
  {
    parameterStack.push(symbol);
  }

  inline shared_ptr<Symbol> pop_parameter()
  {
    shared_ptr<Symbol> symbol = parameterStack.top();
    parameterStack.pop();
    return symbol;
  }

  inline CodeGenVisitor *get_visitor() { return visitor; }

  int getRegisterIndexForSymbol(shared_ptr<Symbol> &param); // Trouve le registre associé à un paramètre

  unsigned int nextFreeSymbolIndex; // Utilisé pour indexer les nouvelles variables

protected:
  int nextBBnumber; // Numéro du prochain bloc (pour le nommage)

  string name;
  Type returnType;
  vector<FunctionParameter> parameterTypes;

  stack<shared_ptr<Symbol>> parameterStack;

  vector<BasicBlock *> bbs;       // Tous les blocs du CFG
  list<SymbolTable> symbolTables; // Pile de tables de symboles (pour la portée)

  CodeGenVisitor *visitor;

  // Fonctions pour l’allocation de registre
  void computeRegisterAllocation();
  InstructionLivenessAnalysis computeLiveInfo();
  RegisterAllocationSpillData determineRegisterAllocationOrder(
      map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>> &interferenceGraph,
      int availableRegisterCount);
  map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>> buildInterferenceGraph(InstructionLivenessAnalysis &liveInfo);
  map<shared_ptr<Symbol>, int> allocateRegisters(
      RegisterAllocationSpillData &spillInfo,
      map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>> &interferenceGraph,
      int availableRegisterCount);
};
