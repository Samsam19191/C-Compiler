#include "IR.h"
#include "CodeGenVisitor.h"
#include "BasicBlock.h"
#include "CFG.h"
#include "Type.h"
#include "ErrorListenerVisitor.h"
#include <iostream>
#include <memory>
#include <queue>
#include <string>

using namespace std;

/**
 * Surcharge de l'opérateur << pour afficher un paramètre
 * Un paramètre peut être soit un Symbol (affiche son lexème) 
 * soit une string (affiche directement la valeur)
 */
ostream &operator<<(ostream &os, const Parameter &parameter)
{
  if (auto symbole = get_if<shared_ptr<Symbol>>(&parameter))
  {
    os << (*symbole)->identifierName; // Affiche le lexème du symbole
  }
  else if (auto n = get_if<string>(&parameter))
  {
    os << *n; // Affiche directement la valeur si c'est une string
  }

  return os;
}

/**
 * Constructeur d'une instruction IR
 * @param basicBlock Le bloc de base contenant l'instruction
 * @param operation L'opération à effectuer 
 * @param type Le type de retour de l'instruction
 * @param parameters Les paramètres de l'instruction
 */
IRInstr::IRInstr(BasicBlock *basicBlock, Operation operation, Type type,
                 const vector<Parameter> &parameters)
    : block(basicBlock), operation(operation), outType(type), parameters(parameters) {}

/**
 * Génère le code assembleur x86-64 correspondant à l'instruction IR
 * @param os Le flux de sortie pour écrire le code assembleur
 * @param cfg Le graphe de flux de contrôle associé
 */
void IRInstr::genAsm(ostream &os, CFG *cfg)
{
  switch (operation)
  {
  case add:
    generateBinaryOperation("addl", os, cfg); // Génère une addition
    break;
  case sub:
    generateBinaryOperation("subl", os, cfg); // Génère une soustraction
    break;
  case mul:
    generateBinaryOperation("imull", os, cfg); // Génère une multiplication
    break;
  case cmpNZ:
    generateCompareNotZero(os, cfg); // Génère une comparaison avec zéro
    break;
  case div:
    generateDivisionInstruction(os, cfg); // Génère une division entière
    break;
  case mod:
    generateModuloInstruction(os, cfg); // Génère un modulo
    break;
  case b_and:
    generateBinaryOperation("andl", os, cfg); // Génère un AND binaire
    break;
  case b_or:
    generateBinaryOperation("orl", os, cfg); // Génère un OR binaire
    break;
  case b_xor:
    generateBinaryOperation("xorl", os, cfg); // Génère un XOR binaire
    break;
  case lt:
    generateComparisonOperation("setl", os, cfg); // Génère une comparaison <
    break;
  case leq:
    generateComparisonOperation("setle", os, cfg); // Génère une comparaison <=
    break;
  case gt:
    generateComparisonOperation("setg", os, cfg); // Génère une comparaison >
    break;
  case geq:
    generateComparisonOperation("setge", os, cfg); // Génère une comparaison >=
    break;
  case eq:
    generateComparisonOperation("sete", os, cfg); // Génère une comparaison ==
    break;
  case neq:
    generateComparisonOperation("setne", os, cfg); // Génère une comparaison !=
    break;
  case ret:
    generateReturnInstruction(os, cfg); // Génère une instruction de retour
    break;
  case var_assign:
    generateVariableAssignment(os, cfg); // Génère une affectation de variable
    break;
  case ldconst:
    generateLoadConstant(os, cfg); // Charge une constante
    break;
  case ldvar:
    generateLoadVariable(os, cfg); // Charge une variable
    break;
  case neg:
    generateUnaryOperation("neg", os, cfg); // Génère une négation
    break;
  case not_:
    generateUnaryOperation("notl", os, cfg); // Génère un NOT binaire
    break;
  case lnot:
    generateUnaryOperation("lnot", os, cfg); // Génère un NOT logique
    break;
  case inc:
    generateUnaryOperation("inc", os, cfg); // Génère une incrémentation
    break;
  case dec:
    generateUnaryOperation("dec", os, cfg); // Génère une décrémentation
    break;
  case nothing:
    break; // Pas d'opération
  case call:
    generateFunctionCall(os, cfg); // Génère un appel de fonction
    break;
  case param:
    generateFunctionParameterPassing(os, cfg); // Prépare un paramètre pour un appel
    break;
  case param_decl:
    break; // Déclaration de paramètre (pas d'implémentation ici)
  }
}

/**
 * Retourne l'ensemble des variables utilisées par cette instruction
 * @return Un set contenant tous les Symboles utilisés comme opérandes
 */
set<shared_ptr<Symbol>> IRInstr::getUsedVariables()
{
  set<shared_ptr<Symbol>> result;
  switch (operation)
  {
  case IRInstr::add:
  case IRInstr::sub:
  case IRInstr::mul:
  case IRInstr::div:
  case IRInstr::mod:
  case IRInstr::b_and:
  case IRInstr::b_or:
  case IRInstr::b_xor:
  case IRInstr::lt:
  case IRInstr::leq:
  case IRInstr::gt:
  case IRInstr::geq:
  case IRInstr::eq:
  case IRInstr::neq:
    result.insert(get<shared_ptr<Symbol>>(parameters[0])); // Ajoute le premier opérande
    result.insert(get<shared_ptr<Symbol>>(parameters[1])); // Ajoute le second opérande
    break;
  case IRInstr::ldconst:
    break; // Pas de variables utilisées
  case IRInstr::var_assign:
  case IRInstr::lnot:
    result.insert(get<shared_ptr<Symbol>>(parameters[1])); // Ajoute la source
    break;
  case IRInstr::cmpNZ:
  case IRInstr::neg:
  case IRInstr::not_:
  case IRInstr::inc:
  case IRInstr::dec:
  case IRInstr::param:
    result.insert(get<shared_ptr<Symbol>>(parameters[0])); // Ajoute la variable
    break;
  case ret:
    if (outType != Type::VOID)
    {
      result.insert(get<shared_ptr<Symbol>>(parameters[0])); // Ajoute la valeur de retour
    }
    break;
  case IRInstr::nothing:
  case IRInstr::ldvar:
  case IRInstr::param_decl:
    break; // Pas de variables utilisées
  case IRInstr::call:
  {
    int parameterCount = parameters.size();
    if (outType != Type::VOID)
    {
      parameterCount--; // Ignore le dernier paramètre si c'est une valeur de retour
    }
    for (int i = 1; i < parameterCount; i++)
    {
      result.insert(get<shared_ptr<Symbol>>(parameters[i])); // Ajoute les paramètres
    }
    break;
  }
  }
  return result;
}

/**
 * Retourne l'ensemble des variables déclarées/modifiées par cette instruction
 * @return Un set contenant tous les Symboles définis par cette instruction  
 */
set<shared_ptr<Symbol>> IRInstr::getDeclaredVariable()
{
  set<shared_ptr<Symbol>> result;
  switch (operation)
  {
  case IRInstr::add:
  case IRInstr::sub:
  case IRInstr::mul:
  case IRInstr::div:
  case IRInstr::mod:
  case IRInstr::b_and:
  case IRInstr::b_or:
  case IRInstr::b_xor:
  case IRInstr::lt:
  case IRInstr::leq:
  case IRInstr::gt:
  case IRInstr::geq:
  case IRInstr::eq:
  case IRInstr::neq:
    result.insert(get<shared_ptr<Symbol>>(parameters[2])); // Ajoute la destination
    break;
  case IRInstr::ldconst:
  case IRInstr::lnot:
    result.insert(get<shared_ptr<Symbol>>(parameters[1])); // Ajoute la destination
    break;
  case IRInstr::var_assign:
  case IRInstr::neg:
  case IRInstr::not_:
  case IRInstr::inc:
  case IRInstr::dec:
  case IRInstr::param_decl:
    result.insert(get<shared_ptr<Symbol>>(parameters[0])); // Ajoute la variable
    break;
  case IRInstr::call:
    if (outType != Type::VOID)
    {
      result.insert(
          get<shared_ptr<Symbol>>(parameters[parameters.size() - 1])); // Ajoute la valeur de retour
    }
    break;
  case IRInstr::ret:
  case IRInstr::cmpNZ:
  case IRInstr::ldvar:
  case IRInstr::nothing:
  case IRInstr::param:
    break; // Pas de variables déclarées
  }
  return result;
}

/**
 * Surcharge de l'opérateur << pour afficher une instruction IR
 * Affiche l'instruction sous une forme lisible de type "a = b + c"
 */
ostream &operator<<(ostream &os, IRInstr &instruction)
{
  switch (instruction.operation)
  {
  case IRInstr::add:
    os << instruction.parameters[2] << " = " << instruction.parameters[0] << " + "
       << instruction.parameters[1];
    break;
  case IRInstr::sub:
    os << instruction.parameters[2] << " = " << instruction.parameters[0] << " - "
       << instruction.parameters[1];
    break;
  case IRInstr::div:
    os << instruction.parameters[2] << " = " << instruction.parameters[0] << " / "
       << instruction.parameters[1];
    break;
  case IRInstr::mod:
    os << instruction.parameters[2] << " = " << instruction.parameters[0] << " % "
       << instruction.parameters[1];
    break;
  case IRInstr::mul:
    os << instruction.parameters[2] << " = " << instruction.parameters[0] << " * "
       << instruction.parameters[1];
    break;
  case IRInstr::lt:
    os << instruction.parameters[2] << " = " << instruction.parameters[0] << " < "
       << instruction.parameters[1];
    break;
  case IRInstr::leq:
    os << instruction.parameters[2] << " = " << instruction.parameters[0]
       << " <= " << instruction.parameters[1];
    break;
  case IRInstr::gt:
    os << instruction.parameters[2] << " = " << instruction.parameters[0] << " > "
       << instruction.parameters[1];
    break;
  case IRInstr::geq:
    os << instruction.parameters[2] << " = " << instruction.parameters[0]
       << " >= " << instruction.parameters[1];
    break;
  case IRInstr::eq:
    os << instruction.parameters[2] << " = " << instruction.parameters[0]
       << " == " << instruction.parameters[1];
    break;
  case IRInstr::neq:
    os << instruction.parameters[2] << " = " << instruction.parameters[0]
       << " != " << instruction.parameters[1];
    break;
  case IRInstr::b_and:
    os << instruction.parameters[2] << " = " << instruction.parameters[0] << " & "
       << instruction.parameters[1];
    break;
  case IRInstr::b_or:
    os << instruction.parameters[2] << " = " << instruction.parameters[0] << " | "
       << instruction.parameters[1];
    break;
  case IRInstr::b_xor:
    os << instruction.parameters[2] << " = " << instruction.parameters[0] << " ^ "
       << instruction.parameters[1];
    break;
  case IRInstr::ldconst:
    os << instruction.parameters[1] << " = " << instruction.parameters[0];
    break;
  case IRInstr::ldvar:
    os << "ldvar " << instruction.parameters[0];
    break;
  case IRInstr::ret:
    os << "ret " << instruction.parameters[0];
    break;
  case IRInstr::var_assign:
    os << instruction.parameters[0] << " = " << instruction.parameters[1];
    break;
  case IRInstr::cmpNZ:
    os << instruction.parameters[0] << " !=  0";
    break;
  case IRInstr::neg:
    os << " - " << instruction.parameters[0];
    break;
  case IRInstr::not_:
    os << " ~ " << instruction.parameters[0];
    break;
  case IRInstr::lnot:
    os << instruction.parameters[1] << "= ! " << instruction.parameters[0];
    break;
  case IRInstr::inc:
    os << "++" << instruction.parameters[0];
    break;
  case IRInstr::dec:
    os << "--" << instruction.parameters[0];
    break;
  case IRInstr::nothing:
  case IRInstr::call:
    if (instruction.parameters.size() == 2)
    {
      os << instruction.parameters[1] << " = call " << instruction.parameters[0];
    }
    else
    {
      os << "call " << instruction.parameters[0];
    }
    break;
  case IRInstr::param:
    os << "param " << instruction.parameters[0];
    break;
  case IRInstr::param_decl:
    os << "param_decl " << instruction.parameters[0];
    break;
  }
  return os;
}

/**
 * Génère le code assembleur pour une comparaison avec zéro
 * Utilisé pour les conditions if/while
 */
void IRInstr::generateCompareNotZero(ostream &os, CFG *cfg)
{
  // Récupère le registre associé au premier paramètre
  int firstRegister =
      cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[0]));

  // Si le registre est un registre temporaire, charge la valeur depuis la pile
  if (firstRegister == cfg->scratchRegister)
  {
    os << "movl -" << get<shared_ptr<Symbol>>(parameters[0])->offset
       << "(%rbp), " << registers32[firstRegister] << endl;
  }

  // Effectue une opération de test sur le registre
  os << "testl %" << registers32[firstRegister] << ", %"
     << registers32[firstRegister] << endl;
}

/**
 * Génère le code assembleur pour une division entière
 * Utilise les registres eax et edx pour stocker le résultat
 */
void IRInstr::generateDivisionInstruction(ostream &os, CFG *cfg)
{
  // Récupère les registres associés aux paramètres
  int firstRegister =
      cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[0]));
  int secondRegister =
      cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[1]));
  int destRegister =
      cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[2]));

  // Charge le premier opérande dans eax
  if (firstRegister == cfg->scratchRegister)
  {
    os << "movl -" << get<shared_ptr<Symbol>>(parameters[0])->offset
       << "(%rbp), %" << registers32[firstRegister] << endl;
  }
  os << "movl %" << registers32[firstRegister] << ", %eax" << endl;

  // Initialise edx à 0 pour la division
  os << "movl $0, %edx" << endl;

  // Charge le second opérande si nécessaire
  if (secondRegister == cfg->scratchRegister)
  {
    os << "movl -" << get<shared_ptr<Symbol>>(parameters[1])->offset
       << "(%rbp), " << registers32[secondRegister] << endl;
  }

  // Effectue la division entière
  os << "idivl %" << registers32[secondRegister] << endl;

  // Stocke le résultat dans le registre de destination
  os << "movl %eax, %" << registers32[destRegister] << endl;

  // Si le registre de destination est temporaire, sauvegarde le résultat dans la pile
  if (destRegister == cfg->scratchRegister)
  {
    os << "movl %" << registers32[destRegister] << ", -"
       << get<shared_ptr<Symbol>>(parameters[2])->offset << "(%rbp)"
       << endl;
  }
}

/**
 * Génère le code assembleur pour un modulo
 * Fonctionne comme la division mais conserve le reste dans edx
 */
void IRInstr::generateModuloInstruction(ostream &os, CFG *cfg)
{
  // Récupère les registres associés aux paramètres
  int firstRegister =
      cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[0]));
  int secondRegister =
      cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[1]));
  int destRegister =
      cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[2]));

  // Charge le premier opérande dans eax
  if (firstRegister == cfg->scratchRegister)
  {
    os << "movl -" << get<shared_ptr<Symbol>>(parameters[0])->offset
       << "(%rbp), %" << registers32[firstRegister] << endl;
  }
  os << "movl %" << registers32[firstRegister] << ", %eax" << endl;

  // Initialise edx à 0 pour la division
  os << "movl $0, %edx" << endl;

  // Charge le second opérande si nécessaire
  if (secondRegister == cfg->scratchRegister)
  {
    os << "movl -" << get<shared_ptr<Symbol>>(parameters[1])->offset
       << "(%rbp), %" << registers32[secondRegister] << endl;
  }

  // Effectue la division entière
  os << "idivl %" << registers32[secondRegister] << endl;

  // Stocke le reste (modulo) dans le registre de destination
  os << "movl %edx, %" << registers32[destRegister] << endl;

  // Si le registre de destination est temporaire, sauvegarde le résultat dans la pile
  if (destRegister == cfg->scratchRegister)
  {
    os << "movl %" << registers32[destRegister] << ",-"
       << get<shared_ptr<Symbol>>(parameters[2])->offset << "(%rbp)"
       << endl;
  }
}

/**
 * Génère le code assembleur pour une instruction return
 * Gère à la fois les retours void et non-void
 */
void IRInstr::generateReturnInstruction(ostream &os, CFG *cfg)
{
  // Si la fonction retourne une valeur, charge-la dans eax
  if (outType != Type::VOID)
  {
    int firstRegister =
        cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[0]));

    if (firstRegister == cfg->scratchRegister)
    {
      os << "movl -" << get<shared_ptr<Symbol>>(parameters[0])->offset
         << "(%rbp), %" << registers32[firstRegister] << endl;
    }
    os << "movl %" << registers32[firstRegister] << ", %eax" << endl;
  }

  // Restaure la base de pile et retourne
  os << "popq %rbp\n";
  os << "ret\n";
}

/**
 * Génère le code assembleur pour une affectation de variable
 * Gère différents types (int/char) avec les bonnes instructions mov
 */
void IRInstr::generateVariableAssignment(ostream &os, CFG *cfg)
{
  // Récupère les registres associés à la source et à la destination
  int destRegister =
      cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[0]));
  int sourceRegister =
      cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[1]));
  auto symbole = get<shared_ptr<Symbol>>(parameters[0]);

  // Détermine l'instruction mov appropriée en fonction du type
  string instr = (symbole->type == Type::CHAR ? "movb " : "movl ");
  const string *registers =
      (symbole->type == Type::CHAR ? registers8 : registers32);

  // Charge la source si elle est dans un registre temporaire
  if (sourceRegister == cfg->scratchRegister)
  {
    os << "movl -" << get<shared_ptr<Symbol>>(parameters[1])->offset
       << "(%rbp), %" << registers32[sourceRegister] << endl;
  }

  // Effectue l'affectation entre registres
  if (sourceRegister != destRegister)
  {
    os << "movl %" << registers32[sourceRegister] << ", %"
       << registers32[destRegister] << "\n";
  }

  // Si la destination est un registre temporaire, sauvegarde dans la pile
  if (destRegister == cfg->scratchRegister)
  {
    os << instr << " %" << registers32[destRegister] << ", -" << symbole->offset
       << "(%rbp)" << endl;
  }
}

/**
 * Génère le code assembleur pour charger une constante
 * Utilise l'instruction mov avec une valeur immédiate
 */
void IRInstr::generateLoadConstant(ostream &os, CFG *cfg)
{
  // Récupère le symbole et la valeur de la constante
  auto symbole = get<shared_ptr<Symbol>>(parameters[1]);
  auto value = get<string>(parameters[0]);
  int destRegister = cfg->getRegisterIndexForSymbol(symbole);

  // Détermine l'instruction mov appropriée en fonction du type
  string instr = (symbole->type == Type::CHAR ? "movb" : "movl");
  const string *registers =
      (symbole->type == Type::CHAR ? registers8 : registers32);

  // Charge la constante dans le registre de destination
  os << "movl $" << value << ", %" << registers32[destRegister] << endl;

  // Si le registre de destination est temporaire, sauvegarde dans la pile
  if (destRegister == cfg->scratchRegister)
  {
    os << "movl %" << registers32[destRegister] << ", -"
       << get<shared_ptr<Symbol>>(parameters[1])->offset << "(%rbp)"
       << endl;
  }
}

/**
 * Génère le code assembleur pour charger une variable en mémoire
 * @param os Le flux de sortie pour écrire le code assembleur
 * @param cfg Le graphe de flux de contrôle associé
 */
void IRInstr::generateLoadVariable(ostream &os, CFG *cfg)
{
  // Récupère le symbole et le registre de destination
  auto symbole = get<shared_ptr<Symbol>>(parameters[0]);
  int destRegister = cfg->getRegisterIndexForSymbol(symbole);

  // Détermine l'instruction mov appropriée en fonction du type
  string instr = (symbole->type == Type::CHAR ? "movb" : "movl");
  const string *registers = (symbole->type == Type::CHAR ? registers8 : registers32);

  // Charge la variable depuis la pile vers le registre
  os << instr << " -" << symbole->offset << "(%rbp), %" 
     << registers[destRegister] << endl;
}

/**
 * Génère le code assembleur pour une opération binaire (add, sub, etc)
 * @param operation Le mnémonique assembleur (ex: "addl", "subl")
 * Gère les différents cas d'allocation de registres
 */
void IRInstr::generateBinaryOperation(const string &operation, ostream &os,
                                      CFG *cfg)
{
  // Récupère les registres associés aux paramètres
  int firstRegister =
      cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[0]));
  int secondRegister =
      cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[1]));
  int destRegister =
      cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[2]));

  // Charge les opérandes si nécessaire et effectue l'opération
  if (firstRegister == cfg->scratchRegister)
  {
    os << "movl -" << get<shared_ptr<Symbol>>(parameters[0])->offset
       << "(%rbp), %" << registers32[firstRegister] << endl;
  }
  if (firstRegister == cfg->scratchRegister &&
      secondRegister == cfg->scratchRegister &&
      destRegister == cfg->scratchRegister)
  {
    os << operation << " -" << get<shared_ptr<Symbol>>(parameters[1])->offset
       << "(%rbp), %" << registers32[destRegister] << endl;
  }
  else if (destRegister != secondRegister)
  {
    if (destRegister != firstRegister)
    {
      os << "movl %" << registers32[firstRegister] << ", %"
         << registers32[destRegister] << "\n";
    }
    if (secondRegister == cfg->scratchRegister)
    {
      os << "movl -" << get<shared_ptr<Symbol>>(parameters[1])->offset
         << "(%rbp), %" << registers32[secondRegister] << endl;
    }
    os << operation << " %" << registers32[secondRegister] << ", %"
       << registers32[destRegister] << endl;
  }
  else
  {
    if (secondRegister != cfg->scratchRegister)
    {
      if (firstRegister != cfg->scratchRegister)
      {
        os << "movl %" << registers32[secondRegister] << ", %"
           << registers32[cfg->scratchRegister] << "\n";
        os << "movl %" << registers32[firstRegister] << ", %"
           << registers32[destRegister] << "\n";
        os << operation << " %" << registers32[cfg->scratchRegister] << ", %"
           << registers32[destRegister] << endl;
      }
      else
      {
        os << "xchg %" << registers32[secondRegister] << ", %"
           << registers32[firstRegister] << endl;
        os << operation << " %" << registers32[firstRegister] << ", %"
           << registers32[destRegister] << endl;
      }
    }
    else
    {
      os << "movl -" << get<shared_ptr<Symbol>>(parameters[1])->offset
         << "(%rbp), %" << registers32[secondRegister] << endl;
      if (destRegister != firstRegister)
      {
        os << "movl %" << registers32[firstRegister] << ", %"
           << registers32[destRegister] << "\n";
      }
      os << operation << " %" << registers32[secondRegister] << ", %"
         << registers32[destRegister] << endl;
    }
  }
}

/**
 * Génère le code assembleur pour une opération de comparaison
 * @param operation Le mnémonique assembleur (ex: "setl", "sete")
 * Produit un résultat booléen (0 ou 1) dans le registre de destination
 */
void IRInstr::generateComparisonOperation(const string &operation, ostream &os, CFG *cfg)
{
  // Récupère les registres associés aux paramètres
  int firstRegister =
      cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[0]));
  int secondRegister =
      cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[1]));
  int destRegister =
      cfg->getRegisterIndexForSymbol(get<shared_ptr<Symbol>>(parameters[2]));

  // Effectue la comparaison et stocke le résultat
  if (firstRegister == cfg->scratchRegister &&
      secondRegister == cfg->scratchRegister)
  {
    os << "movl -" << get<shared_ptr<Symbol>>(parameters[0])->offset
       << "(%rbp), %" << registers32[firstRegister] << endl;
    os << "cmp -" << get<shared_ptr<Symbol>>(parameters[1])->offset
       << "(%rbp)"
       << ", %" << registers32[firstRegister] << endl;
    os << "movzbl %" << registers8[cfg->scratchRegister] << ", %"
       << registers32[destRegister] << endl;
  }
  else
  {
    if (firstRegister == cfg->scratchRegister)
    {
      os << "movl -" << get<shared_ptr<Symbol>>(parameters[0])->offset
         << "(%rbp), %" << registers32[firstRegister] << endl;
    }
    else if (secondRegister == cfg->scratchRegister)
    {
      os << "movl -" << get<shared_ptr<Symbol>>(parameters[1])->offset
         << "(%rbp), %" << registers32[secondRegister] << endl;
    }
    os << "cmp %" << registers32[secondRegister] << ", %"
       << registers32[firstRegister] << endl;
    os << operation << " %" << registers8[cfg->scratchRegister] << endl;
    os << "movzbl %" << registers8[cfg->scratchRegister] << ", %"
       << registers32[destRegister] << endl;
  }

  // Si le registre de destination est temporaire, sauvegarde dans la pile
  if (destRegister == cfg->scratchRegister)
  {
    os << "movl %" << registers32[destRegister] << ", -"
       << get<shared_ptr<Symbol>>(parameters[2])->offset << "(%rbp)";
  }
}

/**
 * Retourne l'index du registre alloué pour un symbole donné
 * Retourne le registre scratch si le symbole n'a pas de registre dédié
 */
int CFG::getRegisterIndexForSymbol(shared_ptr<Symbol> &param)
{
  auto paramLocation = registerAssignment.find(param);
  if (paramLocation != registerAssignment.end())
  {
    return paramLocation->second; // Retourne l'index du registre alloué
  }
  return scratchRegister; // Retourne le registre scratch par défaut
}

/**
 * Génère le code assembleur pour une opération unaire (neg, not, etc)
 * @param operation Le mnémonique assembleur (ex: "neg", "notl")
 * Gère les opérations arithmétiques et logiques
 */
void IRInstr::generateUnaryOperation(const string &operation, ostream &os, CFG *cfg)
{
  auto symbole = get<shared_ptr<Symbol>>(parameters[0]);
  int varRegister = cfg->getRegisterIndexForSymbol(symbole);

  // Gestion des opérations d'incrémentation et de décrémentation
  if (operation == "inc" || operation == "dec")
  {
    if (varRegister == cfg->scratchRegister)
    {
      os << "movl -" << symbole->offset << "(%rbp), %"
         << registers32[varRegister] << endl; // Charge la variable depuis la pile
    }
    os << operation << " %" << registers32[varRegister] << "\n"; // Effectue l'opération
    if (varRegister == cfg->scratchRegister)
    {
      os << "movl %" << registers32[varRegister] << ", -" << symbole->offset
         << "(%rbp)" << endl; // Sauvegarde le résultat dans la pile
    }
  }
  // Gestion des opérations de négation et NOT binaire
  if (operation == "neg" || operation == "notl")
  {
    if (varRegister == cfg->scratchRegister)
    {
      os << "movl -" << symbole->offset << "(%rbp), %"
         << registers32[varRegister] << endl; // Charge la variable depuis la pile
    }
    else
    {
      os << "movl %" << registers32[varRegister] << ", %"
         << registers32[cfg->scratchRegister] << endl; // Copie dans le registre scratch
    }
    os << operation << " %" << registers32[cfg->scratchRegister] << "\n"; // Effectue l'opération
    auto destSymbol = get<shared_ptr<Symbol>>(parameters[1]);
    int destRegister = cfg->getRegisterIndexForSymbol(destSymbol);
    if (destRegister == cfg->scratchRegister)
    {
      os << "movl " << registers32[cfg->scratchRegister] << ", -"
         << destSymbol->offset << "(%rbp)" << endl; // Sauvegarde le résultat dans la pile
    }
    else
    {
      os << "movl %" << registers32[cfg->scratchRegister] << ", %"
         << registers32[destRegister] << endl; // Sauvegarde dans le registre de destination
    }
  }
  // Gestion de l'opération NOT logique
  else if (operation == "lnot")
  {
    os << "cmpl $0, %" << registers32[varRegister] << endl; // Compare avec 0
    int varRegister = cfg->getRegisterIndexForSymbol(symbole);
    if (varRegister == cfg->scratchRegister)
    {
      os << "movl %" << registers32[varRegister] << ", -" << symbole->offset
         << "(%rbp)"
         << "\n"; // Sauvegarde dans la pile
    }
    auto destSymbol = get<shared_ptr<Symbol>>(parameters[1]);
    int destRegister = cfg->getRegisterIndexForSymbol(destSymbol);
    if (destRegister == cfg->scratchRegister)
    {
      os << "movl -" << symbole->offset << "(%rbp), %"
         << registers32[destRegister] << endl; // Charge dans le registre de destination
    }
    os << "sete %" << registers8[destRegister] << endl; // Définit le résultat (0 ou 1)
    os << "movzbl %" << registers8[destRegister] << ", %"
       << registers32[destRegister] << endl; // Étend le résultat à 32 bits
    if (destRegister == cfg->scratchRegister)
    {
      os << "movl %" << registers32[destRegister] << ", -" << symbole->offset
         << "(%rbp)" << endl; // Sauvegarde dans la pile
    }
  }
}

/**
 * Génère le code assembleur pour un appel de fonction
 * Gère:
 * - L'empilement des paramètres
 * - L'appel proprement dit  
 * - La récupération de la valeur de retour
 * - Le nettoyage de la pile
 */
void IRInstr::generateFunctionCall(ostream &os, CFG *cfg)
{
  string functionName = get<string>(parameters[0]); // Nom de la fonction appelée
  CFG *function = cfg->get_visitor()->getFunction(functionName);
  int parameterCount = function->get_parameters_type().size(); // Nombre de paramètres

  // Aligne la pile si nécessaire
  int value = (cfg->nextFreeSymbolIndex + 16 - 1) / 16 * 16;
  if (value)
  {
    os << "subq $" << value << ", %rsp" << endl;
  }
  // Sauvegarde les registres utilisés
  for (int i = 0; i < 8; i++)
  {
    os << "pushq %" << registers64[i] << endl;
  }

  // Gestion des échanges de registres pour les paramètres
  bool exchange = false;
  if (parameterCount >= 6)
  {
    auto symbol8 = get<shared_ptr<Symbol>>(parameters[5]);
    auto symbol9 = get<shared_ptr<Symbol>>(parameters[6]);
    if (cfg->getRegisterIndexForSymbol(symbol8) == 1 && cfg->getRegisterIndexForSymbol(symbol9) == 0)
    {
      os << "xchg %r8d, %r9d" << endl;
      exchange = true;
    }
  }
  // Empile les paramètres
  for (int i = parameterCount - 1; i >= 0; i--)
  {
    if ((i == 4 || i == 5) && exchange)
    {
      continue;
    }
    auto symbole = get<shared_ptr<Symbol>>(parameters[i + 1]);
    int paramRegister = cfg->getRegisterIndexForSymbol(symbole);
    if (paramRegister == cfg->scratchRegister)
    {
      os << "movl -" << symbole->offset << "(%rbp), %"
         << registers32[paramRegister] << endl;
    }
    if (i < 6)
    {
      os << "movl %" << registers32[paramRegister] << ", %" << paramRegisters[i]
         << endl;
    }
    else
    {
      os << "pushq %" << registers64[paramRegister] << endl;
    }
  }

  auto returnVar = get<shared_ptr<Symbol>>(*parameters.rbegin());

  os << "call " << functionName << endl; // Appelle la fonction
  int paramRegister = cfg->getRegisterIndexForSymbol(returnVar);

  // Restaure les registres sauvegardés
  for (int i = 0; i < 8; i++)
  {
    os << "popq %" << registers64[7 - i] << endl;
  }

  for (int i = 7; i <= parameterCount; i++)
  {
    os << "popq %" << registers64[cfg->scratchRegister] << endl;
  }
  if (value)
  {
    os << "addq $" << value << ", %rsp" << endl;
  }

  // Récupère la valeur de retour si nécessaire
  if (outType != Type::VOID)
  {
    os << "movl %eax, %" << registers32[paramRegister] << endl;
    os << "movl %" << registers32[paramRegister] << ", -" << returnVar->offset
       << "(%rbp)" << endl;
  }

  if (outType != Type::VOID && paramRegister != cfg->scratchRegister)
  {
    os << "movl -" << returnVar->offset << "(%rbp)"
       << ", %" << registers32[paramRegister] << endl;
  }
}

/**
 * Prépare le passage d'un paramètre pour un appel de fonction
 * L'empilement effectif sera fait par generateFunctionCall
 */
void IRInstr::generateFunctionParameterPassing(ostream &os, CFG *cfg)
{
  cfg->push_parameter(get<shared_ptr<Symbol>>(parameters[0])); // Ajoute le paramètre
}