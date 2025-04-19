#include "IR.h"
#include "CodeGenVisitor.h"
#include "Type.h"
#include "VisitorErrorListener.h"
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
  if (auto symbol = get_if<shared_ptr<Symbol>>(&parameter))
  {
    os << (*symbol)->lexeme; // Affiche le lexème du symbole
  }
  else if (auto n = get_if<string>(&parameter))
  {
    os << *n; // Affiche directement la valeur si c'est une string
  }

  return os;
}

/**
 * Constructeur d'une instruction IR
 * @param bb_ Le bloc de base contenant l'instruction
 * @param op L'opération à effectuer 
 * @param t Le type de retour de l'instruction
 * @param parameters Les paramètres de l'instruction
 */
IRInstr::IRInstr(BasicBlock *bb_, Operation op, Type t,
                 const vector<Parameter> &parameters)
    : block(bb_), op(op), outType(t), parameters(parameters) {}

/**
 * Génère le code assembleur x86-64 correspondant à l'instruction IR
 * @param os Le flux de sortie pour écrire le code assembleur
 * @param cfg Le graphe de flux de contrôle associé
 */
void IRInstr::genAsm(ostream &os, CFG *cfg)
{
  switch (op)
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
  switch (op)
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
    int cnt = parameters.size();
    if (outType != Type::VOID)
    {
      cnt--; // Ignore le dernier paramètre si c'est une valeur de retour
    }
    for (int i = 1; i < cnt; i++)
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
  switch (op)
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
  switch (instruction.op)
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
  auto symbol = get<shared_ptr<Symbol>>(parameters[0]);

  // Détermine l'instruction mov appropriée en fonction du type
  string instr = (symbol->type == Type::CHAR ? "movb " : "movl ");
  const string *registers =
      (symbol->type == Type::CHAR ? registers8 : registers32);

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
    os << instr << " %" << registers32[destRegister] << ", -" << symbol->offset
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
  auto symbol = get<shared_ptr<Symbol>>(parameters[1]);
  auto val = get<string>(parameters[0]);
  int destRegister = cfg->getRegisterIndexForSymbol(symbol);

  // Détermine l'instruction mov appropriée en fonction du type
  string instr = (symbol->type == Type::CHAR ? "movb" : "movl");
  const string *registers =
      (symbol->type == Type::CHAR ? registers8 : registers32);

  // Charge la constante dans le registre de destination
  os << "movl $" << val << ", %" << registers32[destRegister] << endl;

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
  auto symbol = get<shared_ptr<Symbol>>(parameters[0]);
  int destRegister = cfg->getRegisterIndexForSymbol(symbol);

  // Détermine l'instruction mov appropriée en fonction du type
  string instr = (symbol->type == Type::CHAR ? "movb" : "movl");
  const string *registers = (symbol->type == Type::CHAR ? registers8 : registers32);

  // Charge la variable depuis la pile vers le registre
  os << instr << " -" << symbol->offset << "(%rbp), %" 
     << registers[destRegister] << endl;
}

/**
 * Génère le code assembleur pour une opération binaire (add, sub, etc)
 * @param op Le mnémonique assembleur (ex: "addl", "subl")
 * Gère les différents cas d'allocation de registres
 */
void IRInstr::generateBinaryOperation(const string &op, ostream &os,
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
    os << op << " -" << get<shared_ptr<Symbol>>(parameters[1])->offset
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
    os << op << " %" << registers32[secondRegister] << ", %"
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
        os << op << " %" << registers32[cfg->scratchRegister] << ", %"
           << registers32[destRegister] << endl;
      }
      else
      {
        os << "xchg %" << registers32[secondRegister] << ", %"
           << registers32[firstRegister] << endl;
        os << op << " %" << registers32[firstRegister] << ", %"
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
      os << op << " %" << registers32[secondRegister] << ", %"
         << registers32[destRegister] << endl;
    }
  }
}

/**
 * Génère le code assembleur pour une opération de comparaison
 * @param op Le mnémonique assembleur (ex: "setl", "sete")
 * Produit un résultat booléen (0 ou 1) dans le registre de destination
 */
void IRInstr::generateComparisonOperation(const string &op, ostream &os, CFG *cfg)
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
    os << op << " %" << registers8[cfg->scratchRegister] << endl;
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
 * @param op Le mnémonique assembleur (ex: "neg", "notl")
 * Gère les opérations arithmétiques et logiques
 */
void IRInstr::generateUnaryOperation(const string &op, ostream &os, CFG *cfg)
{
  auto symbol = get<shared_ptr<Symbol>>(parameters[0]);
  int varRegister = cfg->getRegisterIndexForSymbol(symbol);

  // Gestion des opérations d'incrémentation et de décrémentation
  if (op == "inc" || op == "dec")
  {
    if (varRegister == cfg->scratchRegister)
    {
      os << "movl -" << symbol->offset << "(%rbp), %"
         << registers32[varRegister] << endl; // Charge la variable depuis la pile
    }
    os << op << " %" << registers32[varRegister] << "\n"; // Effectue l'opération
    if (varRegister == cfg->scratchRegister)
    {
      os << "movl %" << registers32[varRegister] << ", -" << symbol->offset
         << "(%rbp)" << endl; // Sauvegarde le résultat dans la pile
    }
  }
  // Gestion des opérations de négation et NOT binaire
  if (op == "neg" || op == "notl")
  {
    if (varRegister == cfg->scratchRegister)
    {
      os << "movl -" << symbol->offset << "(%rbp), %"
         << registers32[varRegister] << endl; // Charge la variable depuis la pile
    }
    else
    {
      os << "movl %" << registers32[varRegister] << ", %"
         << registers32[cfg->scratchRegister] << endl; // Copie dans le registre scratch
    }
    os << op << " %" << registers32[cfg->scratchRegister] << "\n"; // Effectue l'opération
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
  else if (op == "lnot")
  {
    os << "cmpl $0, %" << registers32[varRegister] << endl; // Compare avec 0
    int varRegister = cfg->getRegisterIndexForSymbol(symbol);
    if (varRegister == cfg->scratchRegister)
    {
      os << "movl %" << registers32[varRegister] << ", -" << symbol->offset
         << "(%rbp)"
         << "\n"; // Sauvegarde dans la pile
    }
    auto destSymbol = get<shared_ptr<Symbol>>(parameters[1]);
    int destRegister = cfg->getRegisterIndexForSymbol(destSymbol);
    if (destRegister == cfg->scratchRegister)
    {
      os << "movl -" << symbol->offset << "(%rbp), %"
         << registers32[destRegister] << endl; // Charge dans le registre de destination
    }
    os << "sete %" << registers8[destRegister] << endl; // Définit le résultat (0 ou 1)
    os << "movzbl %" << registers8[destRegister] << ", %"
       << registers32[destRegister] << endl; // Étend le résultat à 32 bits
    if (destRegister == cfg->scratchRegister)
    {
      os << "movl %" << registers32[destRegister] << ", -" << symbol->offset
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
  string funcName = get<string>(parameters[0]); // Nom de la fonction appelée
  CFG *function = cfg->get_visitor()->getFunction(funcName);
  int paramNum = function->get_parameters_type().size(); // Nombre de paramètres

  // Aligne la pile si nécessaire
  int val = (cfg->nextFreeSymbolIndex + 16 - 1) / 16 * 16;
  if (val)
  {
    os << "subq $" << val << ", %rsp" << endl;
  }
  // Sauvegarde les registres utilisés
  for (int i = 0; i < 8; i++)
  {
    os << "pushq %" << registers64[i] << endl;
  }

  // Gestion des échanges de registres pour les paramètres
  bool exchange = false;
  if (paramNum >= 6)
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
  for (int i = paramNum - 1; i >= 0; i--)
  {
    if ((i == 4 || i == 5) && exchange)
    {
      continue;
    }
    auto symbol = get<shared_ptr<Symbol>>(parameters[i + 1]);
    int paramRegister = cfg->getRegisterIndexForSymbol(symbol);
    if (paramRegister == cfg->scratchRegister)
    {
      os << "movl -" << symbol->offset << "(%rbp), %"
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

  os << "call " << funcName << endl; // Appelle la fonction
  int paramRegister = cfg->getRegisterIndexForSymbol(returnVar);

  // Restaure les registres sauvegardés
  for (int i = 0; i < 8; i++)
  {
    os << "popq %" << registers64[7 - i] << endl;
  }

  for (int i = 7; i <= paramNum; i++)
  {
    os << "popq %" << registers64[cfg->scratchRegister] << endl;
  }
  if (val)
  {
    os << "addq $" << val << ", %rsp" << endl;
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

/**
 * Constructeur d'un bloc de base
 * @param cfg Le CFG parent
 * @param entry_label Le label assembleur pour ce bloc
 */
BasicBlock::BasicBlock(CFG *cfg, string entry_label)
    : cfg(cfg), label(move(entry_label)), exit_true(nullptr),
      exit_false(nullptr), visited(false) {}

/**
 * Génère le code assembleur pour tout le bloc
 * @param o Le flux de sortie pour écrire le code
 */
void BasicBlock::gen_asm(ostream &o)
{
  if (visited)
  {
    return; // Évite les boucles infinies
  }
  visited = true;
  if (!label.empty())
  {
    cout << label << ":\n"; // Affiche le label du bloc
  }
  for (auto &instruction : instrs)
  {
    instruction.genAsm(o, cfg); // Génère le code pour chaque instruction
  }
  if (exit_false != nullptr)
  {
    o << "je " << exit_false->label << "\n"; // Saut conditionnel
  }
  if (exit_true != nullptr && !exit_true->label.empty())
  {
    o << "jmp " << exit_true->label << "\n"; // Saut inconditionnel
  }
  if (exit_true != nullptr)
  {
    exit_true->gen_asm(o); // Génère le code pour le bloc suivant
  }
  if (exit_false != nullptr)
  {
    exit_false->gen_asm(o); // Génère le code pour le bloc suivant
  }
}

/**
 * Ajoute une instruction IR au bloc
 * @param op L'opération à effectuer
 * @param t Le type de retour
 * @param parameters Les paramètres de l'instruction
 * @return Le symbole résultat pour les instructions qui en produisent un
 */
shared_ptr<Symbol> BasicBlock::add_IRInstr(IRInstr::Operation op, Type t,
                                                vector<Parameter> parameters)
{
  switch (op)
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
  case IRInstr::cmpNZ:
  case IRInstr::neg:
  case IRInstr::not_:
  case IRInstr::ldconst:
  case IRInstr::lnot:
  {
    shared_ptr<Symbol> symbol = cfg->create_new_tempvar(t); // Crée une variable temporaire
    parameters.push_back(symbol); // Ajoute la variable temporaire aux paramètres
    instrs.emplace_back(this, op, t, parameters); // Ajoute l'instruction au bloc
    return symbol; // Retourne la variable temporaire
    break;
  }
  case IRInstr::call:
  {
    if (t != Type::VOID)
    {
      shared_ptr<Symbol> symbol = cfg->create_new_tempvar(t); // Crée une variable temporaire
      parameters.push_back(symbol); // Ajoute la variable temporaire aux paramètres
      instrs.emplace_back(this, op, t, parameters); // Ajoute l'instruction au bloc
      return symbol; // Retourne la variable temporaire
    }
    else
    {
      instrs.emplace_back(this, op, t, parameters); // Ajoute l'instruction au bloc
    }
    break;
  }
  case IRInstr::ret:
  case IRInstr::param:
  case IRInstr::var_assign:
  {
    instrs.emplace_back(this, op, t, parameters); // Ajoute l'instruction au bloc
    break;
  }
  case IRInstr::inc:
  case IRInstr::dec:
    instrs.emplace_back(this, op, t, parameters); // Ajoute l'instruction au bloc
    return get<shared_ptr<Symbol>>(parameters[0]); // Retourne la variable modifiée
    break;

  case IRInstr::param_decl:
    instrs.emplace_back(this, op, t, parameters); // Ajoute l'instruction au bloc
    return get<shared_ptr<Symbol>>(parameters[0]); // Retourne la variable déclarée
  case IRInstr::ldvar:
    return get<shared_ptr<Symbol>>(parameters[0]); // Retourne la variable chargée
    break;
  case IRInstr::nothing:
    break;
  }

  return nullptr; // Retourne nullptr si aucune variable n'est produite
}


/**
 * Constructeur du CFG
 * @param type Type de retour de la fonction
 * @param name Nom de la fonction
 * @param argCount Nombre d'arguments
 * @param visitor Le visiteur de génération de code associé
 */
CFG::CFG(Type type, const string &name, int argCount,
     CodeGenVisitor *visitor)
  : nextFreeSymbolIndex(1 + 4 * max(0, argCount - 6)), name(name),
    returnType(type), visitor(visitor)
{
  add_bb(new BasicBlock(this, "")); // Ajoute un bloc de base initial
  push_table(); // Crée une nouvelle table de symboles pour la portée
}

CFG::~CFG()
{
  // Vide toutes les tables de symboles
  while (!symbolTables.empty())
  {
  pop_table();
  }
  // Libère la mémoire des blocs de base
  for (auto bb : bbs)
  {
  delete bb;
  }
}

/**
 * Ajoute un bloc de base au CFG
 * Le bloc devient le bloc courant
 */
void CFG::add_bb(BasicBlock *bb)
{
  bbs.push_back(bb); // Ajoute le bloc à la liste des blocs
  current_bb = bb; // Définit le bloc courant
}

/**
 * Convertit un registre IR en nom de registre assembleur
 * @param reg Le nom du registre IR (ex: "reg1")
 * @return Le nom du registre assembleur correspondant (ex: "%eax")
 */
string CFG::IR_reg_to_asm(string reg) 
{
  // Vérifie si c'est un registre 32 bits
  if (reg.find("reg") != string::npos) {
    int regNum = stoi(reg.substr(3));
    if (regNum >= 0 && regNum < 8) {
      return "%" + registers32[regNum];
    }
  }
  // Si ce n'est pas un registre connu, retourne le nom tel quel
  return reg;
}

/**
 * Génère le prologue de la fonction (entête, sauvegarde base de pile, etc)
 */
void CFG::gen_asm_prologue(ostream &o)
{
#ifdef __APPLE__
  o << ".globl _" << name << "\n"; // Déclare la fonction comme globale (MacOS)
  o << "_" << name << " : \n";
#else
  o << ".globl " << name << "\n"; // Déclare la fonction comme globale (Linux/Windows)
  o << name << " : \n";
#endif
  o << "pushq %rbp\n"; // Sauvegarde la base de pile
  o << "movq %rsp, %rbp\n"; // Initialise la nouvelle base de pile

  bool isSwapped = false;
  // Gestion des paramètres si plus de 6
  if (parameterTypes.size() >= 6)
  {
  int parameterRegister1 = getRegisterIndexForSymbol(parameterTypes[4].symbol);
  int parameterRegister2 = getRegisterIndexForSymbol(parameterTypes[4].symbol);
  if (parameterRegister1 == 5 && parameterRegister2 == 4)
  {
    o << "xchg %r8d, %r9d" << endl; // Échange les registres si nécessaire
  }
  }

  // Charge les paramètres dans les registres ou la pile
  for (int i = 4; !isSwapped && i < min((int)parameterTypes.size(), 6);
     i++)
  {
  auto parameter = parameterTypes[i];
  int parameterRegister = getRegisterIndexForSymbol(parameter.symbol);
  o << "movl %" << paramRegisters[i] << ", %"
    << registers32[parameterRegister] << endl;
  if (parameterRegister == scratchRegister)
  {
    o << "movl %" << registers32[parameterRegister] << ", -"
    << parameter.symbol->offset << "(%rbp)" << endl;
  }
  }
  for (int i = 0; i < parameterTypes.size(); i++)
  {
  if (i == 4 || i == 5)
  {
    continue;
  }
  auto parameter = parameterTypes[i];
  int parameterRegister = getRegisterIndexForSymbol(parameter.symbol);
  if (i < 6)
  {
    o << "movl %" << paramRegisters[i] << ", %"
    << registers32[parameterRegister] << endl;
  }
  else
  {
    o << "movl " << 8 * (i - 4) << "(%rbp)"
    << ", %" << registers32[parameterRegister] << endl;
  }
  if (parameterRegister == scratchRegister)
  {
    o << "movl %" << registers32[parameterRegister] << ", -"
    << parameter.symbol->offset << "(%rbp)" << endl;
  }
  }
}

/**
 * Génère le code assembleur pour toute la fonction
 * Effectue d'abord l'allocation de registres puis génère le code
 */
void CFG::gen_asm(ostream &o)
{
  computeRegisterAllocation(); // Effectue l'allocation des registres
  gen_asm_prologue(o); // Génère le prologue
  bbs[0]->gen_asm(o); // Génère le code des blocs de base
  gen_asm_epilogue(o); // Génère l'épilogue
}

/**
 * Génère l'épilogue de la fonction (nettoyage de pile et retour)
 * @param o Le flux de sortie pour écrire le code assembleur
 */
void CFG::gen_asm_epilogue(ostream &o)
{
  // Restaure la base de pile
  o << "popq %rbp\n";
  
  // Retourne de la fonction
  o << "ret\n";
  
  // Aligne la pile si nécessaire (pour les appels système)
  if (nextFreeSymbolIndex > 0) {
    int stackSize = ((nextFreeSymbolIndex + 15) / 16) * 16;
    o << ".size " << name << ", .-" << name << "\n";
  }
}

/**
 * Pop une  table de symboles pour la portée courante
 */
void CFG::pop_table()
{
  for (auto it = symbolTables.front().begin(); it != symbolTables.front().end();
     it++)
  {
  if (!it->second->used)
  {
    VisitorErrorListener::addError("Variable " + it->first +
                     " not used (declared in line " +
                     to_string(it->second->line) + ")",
                   ErrorType::Warning);
  }
  }
  symbolTables.pop_front(); // Supprime la table de symboles courante
}

/**
 * Crée une nouvelle table de symboles pour la portée courante
 */
bool CFG::add_symbol(string id, Type t, int line)
{
  if (symbolTables.front().count(id))
  {
  return false; // Retourne false si le symbole existe déjà
  }
  shared_ptr<Symbol> newSymbol = make_shared<Symbol>(t, id, line);
  unsigned int sz = getSize(t);
  // This expression handles stack alignment
  newSymbol->offset = (nextFreeSymbolIndex + 2 * (sz - 1)) / sz * sz;
  nextFreeSymbolIndex += sz;
  symbolTables.front()[id] = newSymbol;

  return true;
}

shared_ptr<Symbol> CFG::get_symbol(const string &name)
{
  auto it = symbolTables.begin();
  while (it != symbolTables.end())
  {
  auto symbol = it->find(name);
  if (symbol != it->end())
  {
    return symbol->second; // Retourne le symbole s'il est trouvé
  }
  it++;
  }
  return nullptr; // Retourne nullptr si le symbole n'est pas trouvé
}

shared_ptr<Symbol> CFG::create_new_tempvar(Type t)
{
  unsigned int sz = getSize(t);
  unsigned int offset = (nextFreeSymbolIndex + 2 * (sz - 1)) / sz * sz;
  string tempVarName = "!T" + to_string(offset);
  if (add_symbol(tempVarName, t, 0))
  {
  shared_ptr<Symbol> symbol = get_symbol(tempVarName);
  symbol->used = true;
  return symbol; // Retourne la variable temporaire créée
  }
  return nullptr;
}

shared_ptr<Symbol> CFG::add_parameter(const string &name, Type type,
                       int line)
{
  bool new_symbol = add_symbol(name, type, line);
  if (!new_symbol)
  {
  VisitorErrorListener::addError(
    "A parameter with name " + name + " has already been declared", line);
  }
  auto symbol = get_symbol(name);
  parameterTypes.emplace_back(type, symbol);
  return symbol;
}

/**
 * Retourne l'offset d'une variable dans la pile
 * @param name Le nom de la variable
 * @return L'offset (décalage) par rapport à %rbp où la variable est stockée
 */
int CFG::get_var_index(string name)
{
  shared_ptr<Symbol> symbol = get_symbol(name);
  if (symbol != nullptr) {
    return symbol->offset;
  }
  // Retourne -1 si la variable n'existe pas
  return -1; 
}

/**
 * Retourne le type d'une variable
 * @param name Le nom de la variable
 * @return Le type de la variable (INT, CHAR, etc.)
 */
Type CFG::get_var_type(string name)
{
  shared_ptr<Symbol> symbol = get_symbol(name);
  if (symbol != nullptr) {
    return symbol->type;
  }
  // Retourne VOID si la variable n'existe pas
  return Type::VOID;
}

/**
 * Effectue l'analyse de vivacité des instructions
 * @return Les informations de vivacité pour chaque instruction
 */
/**
 * Effectue l'analyse de vivacité des instructions dans le CFG.
 * Cette analyse détermine les variables vivantes avant et après chaque instruction.
 * @return Les informations de vivacité pour chaque instruction.
 */
InstructionLivenessAnalysis CFG::computeLiveInfo()
{
  InstructionLivenessAnalysis liveInfo;
  bool stable = false;

  // Répète jusqu'à ce que l'analyse devienne stable (aucun changement dans les ensembles de vivacité)
  while (!stable)
  {
    stable = true;
    set<BasicBlock *> visitedBB; // Ensemble des blocs visités
    stack<BasicBlock *> visitOrder; // Ordre de visite des blocs
    visitOrder.push(bbs[0]); // Commence par le premier bloc de base

    // Parcourt les blocs de base dans l'ordre de visite
    while (!visitOrder.empty())
    {
      BasicBlock *currentBB = visitOrder.top();
      visitOrder.pop();
      visitedBB.insert(currentBB);

      int instructionIndex = 0;

      // Parcourt les instructions du bloc courant
      for (auto &instr : currentBB->instrs)
      {
        set<shared_ptr<Symbol>> oldIn; // Variables vivantes avant l'instruction (ancienne version)
        set<shared_ptr<Symbol>> oldOut; // Variables vivantes après l'instruction (ancienne version)

        // Récupère les ensembles de vivacité actuels
        auto inPtr = liveInfo.liveVariablesBeforeInstruction.find(&instr);
        auto outPtr = liveInfo.liveVariablesAfterInstruction.find(&instr);
        if (inPtr != liveInfo.liveVariablesBeforeInstruction.end())
        {
          oldIn = inPtr->second;
        }
        if (outPtr != liveInfo.liveVariablesAfterInstruction.end())
        {
          oldOut = outPtr->second;
        }

        // Calcule les nouvelles variables vivantes avant l'instruction
        set<shared_ptr<Symbol>> inUnion = oldOut;
        liveInfo.liveVariablesBeforeInstruction[&instr] = instr.getUsedVariables();
        for (auto &toRemove : instr.getDeclaredVariable())
        {
          inUnion.erase(toRemove); // Supprime les variables déclarées
        }
        for (auto &toAdd : inUnion)
        {
          liveInfo.liveVariablesBeforeInstruction[&instr].insert(toAdd);
        }

        // Calcule les nouvelles variables vivantes après l'instruction
        liveInfo.liveVariablesAfterInstruction[&instr].clear();
        vector<IRInstr *> nextInstrs;

        // Ajoute la prochaine instruction du bloc si elle existe
        if (instructionIndex + 1 < currentBB->instrs.size())
        {
          nextInstrs.push_back(&currentBB->instrs[instructionIndex + 1]);
        }
        else
        {
          // Effectue une recherche BFS pour trouver les prochaines instructions possibles
          set<BasicBlock *> bfsBB;
          queue<BasicBlock *> visitOrder;
          if (currentBB->exit_true != nullptr)
          {
            visitOrder.push(currentBB->exit_true);
          }
          if (currentBB->exit_false != nullptr)
          {
            visitOrder.push(currentBB->exit_false);
          }
          while (!visitOrder.empty())
          {
            BasicBlock *currentVisitedBB = visitOrder.front();
            visitOrder.pop();
            bfsBB.insert(currentVisitedBB);
            if (currentVisitedBB->instrs.size() > 0)
            {
              nextInstrs.push_back(&currentVisitedBB->instrs[0]);
            }
            else
            {
              if (currentVisitedBB->exit_true != nullptr)
              {
                visitOrder.push(currentVisitedBB->exit_true);
              }
              if (currentVisitedBB->exit_false != nullptr)
              {
                visitOrder.push(currentVisitedBB->exit_false);
              }
            }
          }
        }

        // Met à jour les variables vivantes après l'instruction
        for (auto &nextInstruction : nextInstrs)
        {
          for (auto var : liveInfo.liveVariablesBeforeInstruction[nextInstruction])
          {
            liveInfo.liveVariablesAfterInstruction[&instr].insert(var);
          }
        }

        // Vérifie si l'analyse est stable
        if (stable && (liveInfo.liveVariablesBeforeInstruction[&instr] != oldIn ||
                       liveInfo.liveVariablesAfterInstruction[&instr] != oldOut))
        {
          stable = false;
        }

        instructionIndex++;
      }

      // Ajoute les blocs de sortie à visiter
      if (currentBB->exit_true != nullptr &&
          visitedBB.find(currentBB->exit_true) == visitedBB.end())
      {
        visitOrder.push(currentBB->exit_true);
      }
      if (currentBB->exit_false != nullptr &&
          visitedBB.find(currentBB->exit_false) == visitedBB.end())
      {
        visitOrder.push(currentBB->exit_false);
      }
    }
  }

  return liveInfo;
}

/**
 * Calcule le nombre de voisins non encore utilisés dans le graphe d'interférence.
 * @param neighbors Liste des voisins d'un nœud.
 * @param usedNodes Ensemble des nœuds déjà utilisés.
 * @return Le nombre de voisins non encore utilisés.
 */
int computeNeighbors(vector<shared_ptr<Symbol>> &neighbors,
                     set<shared_ptr<Symbol>> &usedNodes)
{
  int neighborCount = 0;
  for (auto x : neighbors)
  {
    if (find(usedNodes.begin(), usedNodes.end(), x) == usedNodes.end())
    {
      neighborCount++;
    }
  }
  return neighborCount;
}

/**
 * Trouve l'ordre d'allocation des registres et les variables à décharger
 * @param interferenceGraph Le graphe d'interférence
 * @param registerCount Le nombre de registres disponibles
 * @return Les informations sur les variables à décharger et l'ordre d'allocation
 */
RegisterAllocationSpillData CFG::findColorOrder(
  map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>>
    &interferenceGraph,
  int registerCount)
{
  int n = interferenceGraph.size();
  RegisterAllocationSpillData spillInfo;
  set<shared_ptr<Symbol>> usedNodes;
  int unselectedNodes = 0;
  while (unselectedNodes < n)
  {
  bool foundNode = false;
  for (auto node = interferenceGraph.begin(); node != interferenceGraph.end();
     node++)
  {
    if (usedNodes.find(node->first) == usedNodes.end() &&
      computeNeighbors(node->second, usedNodes) < registerCount)
    {
    spillInfo.registerAssignmentOrder.push(node->first);
    usedNodes.insert(node->first);
    foundNode = true;
    break;
    }
  }
  if (!foundNode)
  {
    // Just spill the first variable
    for (auto node = interferenceGraph.begin();
       node != interferenceGraph.end(); node++)
    {
    if (find(usedNodes.begin(), usedNodes.end(), node->first) ==
      usedNodes.end())
    {
      usedNodes.insert(node->first);
      spillInfo.variablesThatWereSpilled.insert(node->first);
      break;
    }
    }
  }
  unselectedNodes++;
  }
  return spillInfo;
}


map<shared_ptr<Symbol>, int> CFG::assignRegisters(
  RegisterAllocationSpillData &spillInfo,
  map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>>
    &interferenceGraph,
  int registerCount)
{
  int n = interferenceGraph.size();
  map<shared_ptr<Symbol>, int> color;
  while (!spillInfo.registerAssignmentOrder.empty())
  {
  auto currentNode = spillInfo.registerAssignmentOrder.top();
  spillInfo.registerAssignmentOrder.pop();
  for (int curColor = 0; curColor < registerCount; curColor++)
  {
    bool colorAvailable = true;
    for (auto x : interferenceGraph[currentNode])
    {
    if (color.find(x) != color.end() && color[x] == curColor)
    {
      colorAvailable = false;
      break;
    }
    }
    if (colorAvailable)
    {
    color[currentNode] = curColor;
    break;
    }
  }
  }
  return color;
}

/**
 * Construit le graphe d'interférence à partir des informations de vivacité
 * @param liveInfo Les informations de vivacité
 * @return Le graphe d'interférence
 */
map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>>
CFG::buildInterferenceGraph(InstructionLivenessAnalysis &liveInfo)
{
  map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>>
    interferenceGraph;
  for (auto inPtr : liveInfo.liveVariablesBeforeInstruction)
  {
  auto declaredVar = inPtr.first->getDeclaredVariable();
  if (!declaredVar.empty())
  {
    auto definedVariable = *declaredVar.begin();
    if (interferenceGraph.find(definedVariable) == interferenceGraph.end())
    {
    interferenceGraph[definedVariable] =
      vector<shared_ptr<Symbol>>();
    }
    for (auto &outVar : liveInfo.liveVariablesAfterInstruction[inPtr.first])
    {
    if (outVar != definedVariable &&
      find(interferenceGraph[definedVariable].begin(),
            interferenceGraph[definedVariable].end(),
            outVar) == interferenceGraph[definedVariable].end())
    {
      interferenceGraph[definedVariable].push_back(outVar);
      interferenceGraph[outVar].push_back(definedVariable);
    }
    }
  }
  }
  return interferenceGraph;
}

/**
 * Effectue l'allocation de registres pour le CFG
 * Utilise l'analyse de vivacité et le graphe d'interférence
 */
void CFG::computeRegisterAllocation()
{
  InstructionLivenessAnalysis liveInfo = computeLiveInfo();
  map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>>
    interferenceGraph = buildInterferenceGraph(liveInfo);
  RegisterAllocationSpillData spillInfo = findColorOrder(interferenceGraph, 7);

  registerAssignment = assignRegisters(spillInfo, interferenceGraph, 7);
}