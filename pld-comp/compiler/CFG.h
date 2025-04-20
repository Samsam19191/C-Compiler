#ifndef CFG_H
#define CFG_H

#include <vector>     
#include <memory>      
#include <string>       
#include <ostream>     
#include <map>         
#include <set>          
#include <stack>        
#include <list>        

#include "IR.h"         
#include "Symbol.h"     
#include "Type.h"       
#include "BasicBlock.h" 
#include "CodeGenVisitor.h" 

// ========== Structures auxiliaires ==========

// Représente un paramètre de fonction avec son type et symbole
struct FunctionParameter
{
  Type type;
  shared_ptr<Symbol> symbole;
  FunctionParameter(Type type, shared_ptr<Symbol> symbole)
      : type(type), symbole(symbole) {}
};

// Contient les ensembles de variables vivantes avant et après chaque instruction (analyse de vivacité)
struct InstructionLivenessInfo
{
  map<IRInstr *, set<shared_ptr<Symbol>>> liveVariablesBeforeInstruction;
  map<IRInstr *, set<shared_ptr<Symbol>>> liveVariablesAfterInstruction;
};

// Informations utilisées lors de l'allocation de registres : ordre de traitement et variables spillées
struct RegisterAllocationInfo
{
    stack<shared_ptr<Symbol>> allocationOrder; //allocationOrder
    set<shared_ptr<Symbol>> spilledVariables;        
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

  inline void push_parameter(shared_ptr<Symbol> symbole)
  {
    parameterStack.push(symbole);
  }

  inline shared_ptr<Symbol> pop_parameter()
  {
    shared_ptr<Symbol> symbole = parameterStack.top();
    parameterStack.pop();
    return symbole;
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
  void performRegisterAllocation();
  InstructionLivenessInfo computeLiveInfo();
  RegisterAllocationInfo determineRegisterAllocationOrder(
      map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>> &interferenceGraph,
      int availableRegisterCount);
  map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>> constructInterferenceGraph(InstructionLivenessInfo &livenessInfo);
  map<shared_ptr<Symbol>, int> allocateRegisters(
      RegisterAllocationInfo &registerAllocationInfo,
      map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>> &interferenceGraph,
      int availableRegisterCount);
};

#endif
