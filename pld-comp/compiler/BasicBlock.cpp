#include "BasicBlock.h"
#include "CFG.h"
#include "IR.h"
#include "Symbol.h"
#include "Type.h"
#include "ErrorListenerVisitor.h"
#include <iostream>
#include <memory>
#include <queue>
#include <string>

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
  for (auto &instruction : instructions)
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
 * @param operation L'opération à effectuer
 * @param type Le type de retour
 * @param parameters Les paramètres de l'instruction
 * @return Le symbole résultat pour les instructions qui en produisent un
 */
shared_ptr<Symbol> BasicBlock::add_IRInstr(IRInstr::Operation operation, Type type,
                                                vector<Parameter> parameters)
{
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
  case IRInstr::cmpNZ:
  case IRInstr::neg:
  case IRInstr::not_:
  case IRInstr::ldconst:
  case IRInstr::lnot:
  {
    shared_ptr<Symbol> symbole = cfg->create_new_tempvar(type); // Crée une variable temporaire
    parameters.push_back(symbole); // Ajoute la variable temporaire aux paramètres
    instructions.emplace_back(this, operation, type, parameters); // Ajoute l'instruction au bloc
    return symbole; // Retourne la variable temporaire
    break;
  }
  case IRInstr::call:
  {
    if (type != Type::VOID)
    {
      shared_ptr<Symbol> symbole = cfg->create_new_tempvar(type); // Crée une variable temporaire
      parameters.push_back(symbole); // Ajoute la variable temporaire aux paramètres
      instructions.emplace_back(this, operation, type, parameters); // Ajoute l'instruction au bloc
      return symbole; // Retourne la variable temporaire
    }
    else
    {
      instructions.emplace_back(this, operation, type, parameters); // Ajoute l'instruction au bloc
    }
    break;
  }
  case IRInstr::ret:
  case IRInstr::param:
  case IRInstr::var_assign:
  {
    instructions.emplace_back(this, operation, type, parameters); // Ajoute l'instruction au bloc
    break;
  }
  case IRInstr::inc:
  case IRInstr::dec:
    instructions.emplace_back(this, operation, type, parameters); // Ajoute l'instruction au bloc
    return get<shared_ptr<Symbol>>(parameters[0]); // Retourne la variable modifiée
    break;

  case IRInstr::param_decl:
    instructions.emplace_back(this, operation, type, parameters); // Ajoute l'instruction au bloc
    return get<shared_ptr<Symbol>>(parameters[0]); // Retourne la variable déclarée
  case IRInstr::ldvar:
    return get<shared_ptr<Symbol>>(parameters[0]); // Retourne la variable chargée
    break;
  case IRInstr::nothing:
    break;
  }

  return nullptr; // Retourne nullptr si aucune variable n'est produite
}

