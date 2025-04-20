#include "CFG.h"
#include "BasicBlock.h"
#include "IR.h"
#include "Symbol.h"
#include "Type.h"
#include "ErrorListenerVisitor.h"

#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <variant>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include <algorithm>
#include <utility>
#include <list>

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
 int parameterRegister1 = getRegisterIndexForSymbol(parameterTypes[4].symbole);
 int parameterRegister2 = getRegisterIndexForSymbol(parameterTypes[4].symbole);
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
 int parameterRegister = getRegisterIndexForSymbol(parameter.symbole);
 o << "movl %" << paramRegisters[i] << ", %"
   << registers32[parameterRegister] << endl;
 if (parameterRegister == scratchRegister)
 {
   o << "movl %" << registers32[parameterRegister] << ", -"
   << parameter.symbole->offset << "(%rbp)" << endl;
 }
 }
 for (int i = 0; i < parameterTypes.size(); i++)
 {
 if (i == 4 || i == 5)
 {
   continue;
 }
 auto parameter = parameterTypes[i];
 int parameterRegister = getRegisterIndexForSymbol(parameter.symbole);
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
   << parameter.symbole->offset << "(%rbp)" << endl;
 }
 }
}

/**
* Génère le code assembleur pour toute la fonction
* Effectue d'abord l'allocation de registres puis génère le code
*/
void CFG::gen_asm(ostream &o)
{
 performRegisterAllocation(); // Effectue l'allocation des registres
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
   ErrorListenerVisitor::addError("Variable " + it->first +
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
 auto symbole = it->find(name);
 if (symbole != it->end())
 {
   return symbole->second; // Retourne le symbole s'il est trouvé
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
 shared_ptr<Symbol> symbole = get_symbol(tempVarName);
 symbole->used = true;
 return symbole; // Retourne la variable temporaire créée
 }
 return nullptr;
}

shared_ptr<Symbol> CFG::add_parameter(const string &name, Type type,
                      int line)
{
 bool new_symbol = add_symbol(name, type, line);
 if (!new_symbol)
 {
 ErrorListenerVisitor::addError(
   "A parameter with name " + name + " has already been declared", line);
 }
 auto symbole = get_symbol(name);
 parameterTypes.emplace_back(type, symbole);
 return symbole;
}

/**
* Retourne l'offset d'une variable dans la pile
* @param name Le nom de la variable
* @return L'offset (décalage) par rapport à %rbp où la variable est stockée
*/
int CFG::get_var_index(string name)
{
 shared_ptr<Symbol> symbole = get_symbol(name);
 if (symbole != nullptr) {
   return symbole->offset;
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
 shared_ptr<Symbol> symbole = get_symbol(name);
 if (symbole != nullptr) {
   return symbole->type;
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
InstructionLivenessInfo CFG::computeLiveInfo()
{
 InstructionLivenessInfo livenessInfo;
 bool isAnalysisStable = false;

 // Répète jusqu'à ce que l'analyse devienne stable (aucun changement dans les ensembles de vivacité)
 while (!isAnalysisStable)
 {
    isAnalysisStable = true;
    set<BasicBlock *> visitedBlocks; // Ensemble des blocs visités
    stack<BasicBlock *> blocksToProcess; // Pile des blocs à traiter
    blocksToProcess.push(bbs[0]); // Commence par le premier bloc de base

    // Parcourt les blocs de base dans l'ordre de visite
    while (!blocksToProcess.empty())
    {
      BasicBlock *currentBlock = blocksToProcess.top();
      blocksToProcess.pop();
      visitedBlocks.insert(currentBlock);

      int instructionIndex = 0;

      // Parcourt les instructions du bloc courant
      for (auto &instruction : currentBlock->instructions)
      {
         set<shared_ptr<Symbol>> previousInSet; // Variables vivantes avant l'instruction (ancienne version)
         set<shared_ptr<Symbol>> previousOutSet; // Variables vivantes après l'instruction (ancienne version)

         // Récupère les ensembles de vivacité actuels
         auto inSetIterator = livenessInfo.liveVariablesBeforeInstruction.find(&instruction);
         auto outSetIterator = livenessInfo.liveVariablesAfterInstruction.find(&instruction);
         if (inSetIterator != livenessInfo.liveVariablesBeforeInstruction.end())
         {
            previousInSet = inSetIterator->second;
         }
         if (outSetIterator != livenessInfo.liveVariablesAfterInstruction.end())
         {
            previousOutSet = outSetIterator->second;
         }

         // Calcule les nouvelles variables vivantes avant l'instruction
         set<shared_ptr<Symbol>> outSetUnion = previousOutSet;
         livenessInfo.liveVariablesBeforeInstruction[&instruction] = instruction.getUsedVariables();
         for (auto &declaredVariable : instruction.getDeclaredVariable())
         {
            outSetUnion.erase(declaredVariable); // Supprime les variables déclarées
         }
         for (auto &liveVariable : outSetUnion)
         {
            livenessInfo.liveVariablesBeforeInstruction[&instruction].insert(liveVariable);
         }

         // Calcule les nouvelles variables vivantes après l'instruction
         livenessInfo.liveVariablesAfterInstruction[&instruction].clear();
         vector<IRInstr *> nextInstructions;

         // Ajoute la prochaine instruction du bloc si elle existe
         if (instructionIndex + 1 < currentBlock->instructions.size())
         {
            nextInstructions.push_back(&currentBlock->instructions[instructionIndex + 1]);
         }
         else
         {
            // Effectue une recherche BFS pour trouver les prochaines instructions possibles
            set<BasicBlock *> visitedInBFS;
            queue<BasicBlock *> blocksToVisitInBFS;
            if (currentBlock->exit_true != nullptr)
            {
              blocksToVisitInBFS.push(currentBlock->exit_true);
            }
            if (currentBlock->exit_false != nullptr)
            {
              blocksToVisitInBFS.push(currentBlock->exit_false);
            }
            while (!blocksToVisitInBFS.empty())
            {
              BasicBlock *nextBlock = blocksToVisitInBFS.front();
              blocksToVisitInBFS.pop();
              visitedInBFS.insert(nextBlock);
              if (!nextBlock->instructions.empty())
              {
                 nextInstructions.push_back(&nextBlock->instructions[0]);
              }
              else
              {
                 if (nextBlock->exit_true != nullptr)
                 {
                    blocksToVisitInBFS.push(nextBlock->exit_true);
                 }
                 if (nextBlock->exit_false != nullptr)
                 {
                    blocksToVisitInBFS.push(nextBlock->exit_false);
                 }
              }
            }
         }

         // Met à jour les variables vivantes après l'instruction
         for (auto &nextInstruction : nextInstructions)
         {
            for (auto liveVariable : livenessInfo.liveVariablesBeforeInstruction[nextInstruction])
            {
              livenessInfo.liveVariablesAfterInstruction[&instruction].insert(liveVariable);
            }
         }

         // Vérifie si l'analyse est stable
         if (isAnalysisStable && (livenessInfo.liveVariablesBeforeInstruction[&instruction] != previousInSet ||
                                          livenessInfo.liveVariablesAfterInstruction[&instruction] != previousOutSet))
         {
            isAnalysisStable = false;
         }

         instructionIndex++;
      }

      // Ajoute les blocs de sortie à traiter
      if (currentBlock->exit_true != nullptr &&
            visitedBlocks.find(currentBlock->exit_true) == visitedBlocks.end())
      {
         blocksToProcess.push(currentBlock->exit_true);
      }
      if (currentBlock->exit_false != nullptr &&
            visitedBlocks.find(currentBlock->exit_false) == visitedBlocks.end())
      {
         blocksToProcess.push(currentBlock->exit_false);
      }
    }
 }

 return livenessInfo;
}

/**
* Calcule le nombre de voisins non encore utilisés dans le graphe d'interférence.
* @param neighbors Liste des voisins d'un nœud.
* @param usedNodes Ensemble des nœuds déjà utilisés.
* @return Le nombre de voisins non encore utilisés.
*/
int countUnusedNeighbors(vector<shared_ptr<Symbol>> &neighborSymbols,
            set<shared_ptr<Symbol>> &allocatedNodes)
{
 int unusedNeighborCount = 0;
 for (auto neighbor : neighborSymbols)
 {
 if (allocatedNodes.find(neighbor) == allocatedNodes.end())
 {
   unusedNeighborCount++;
 }
 }
 return unusedNeighborCount;
}

/**
* Détermine l'ordre d'allocation des registres et les variables à décharger
* @param interferenceGraph Le graphe d'interférence
* @param availableRegisterCount Le nombre de registres disponibles
* @return Les informations sur les variables à décharger et l'ordre d'allocation
*/
RegisterAllocationInfo CFG::determineRegisterAllocationOrder(
 map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>>
 &interferenceGraph,
 int availableRegisterCount)
{
 int totalSymbols = interferenceGraph.size();
 RegisterAllocationInfo allocationInfo;
 set<shared_ptr<Symbol>> allocatedSymbols;
 int processedSymbols = 0;

 while (processedSymbols < totalSymbols)
 {
 bool symbolAllocated = false;
 for (auto symbolEntry = interferenceGraph.begin(); symbolEntry != interferenceGraph.end(); symbolEntry++)
 {
    if (allocatedSymbols.find(symbolEntry->first) == allocatedSymbols.end() &&
      countUnusedNeighbors(symbolEntry->second, allocatedSymbols) < availableRegisterCount)
    {
    allocationInfo.allocationOrder.push(symbolEntry->first);
    allocatedSymbols.insert(symbolEntry->first);
    symbolAllocated = true;
    break;
    }
 }
 if (!symbolAllocated)
 {
    // Spill the first unallocated symbol
    for (auto symbolEntry = interferenceGraph.begin(); symbolEntry != interferenceGraph.end(); symbolEntry++)
    {
    if (allocatedSymbols.find(symbolEntry->first) == allocatedSymbols.end())
    {
      allocatedSymbols.insert(symbolEntry->first);
      allocationInfo.spilledVariables.insert(symbolEntry->first);
      break;
    }
    }
 }
 processedSymbols++;
 }
 return allocationInfo;
}

map<shared_ptr<Symbol>, int> CFG::allocateRegisters(
 RegisterAllocationInfo &registerAllocationInfo,
 map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>>
 &interferenceGraph,
 int totalAvailableRegisters)
{
 map<shared_ptr<Symbol>, int> registerAssignments;
 while (!registerAllocationInfo.allocationOrder.empty())
 {
 auto currentSymbol = registerAllocationInfo.allocationOrder.top();
 registerAllocationInfo.allocationOrder.pop();
 for (int registerIndex = 0; registerIndex < totalAvailableRegisters; registerIndex++)
 {
    bool isRegisterFree = true;
    for (auto neighborSymbol : interferenceGraph[currentSymbol])
    {
    if (registerAssignments.find(neighborSymbol) != registerAssignments.end() &&
      registerAssignments[neighborSymbol] == registerIndex)
    {
      isRegisterFree = false;
      break;
    }
    }
    if (isRegisterFree)
    {
    registerAssignments[currentSymbol] = registerIndex;
    break;
    }
 }
 }
 return registerAssignments;
}

/**
* Construit le graphe d'interférence à partir des informations de vivacité
* @param liveInfo Les informations de vivacité
* @return Le graphe d'interférence
*/
map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>>
CFG::constructInterferenceGraph(InstructionLivenessInfo &livenessInfo)
{
    map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>> interferenceGraph;

    for (auto instructionEntry : livenessInfo.liveVariablesBeforeInstruction)
    {
        auto declaredVariables = instructionEntry.first->getDeclaredVariable();
        if (!declaredVariables.empty())
        {
            auto definedVariable = *declaredVariables.begin();
            if (interferenceGraph.find(definedVariable) == interferenceGraph.end())
            {
                interferenceGraph[definedVariable] = vector<shared_ptr<Symbol>>();
            }

            for (auto &liveVariable : livenessInfo.liveVariablesAfterInstruction[instructionEntry.first])
            {
                if (liveVariable != definedVariable &&
                    find(interferenceGraph[definedVariable].begin(),
                         interferenceGraph[definedVariable].end(),
                         liveVariable) == interferenceGraph[definedVariable].end())
                {
                    interferenceGraph[definedVariable].push_back(liveVariable);
                    interferenceGraph[liveVariable].push_back(definedVariable);
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
void CFG::performRegisterAllocation()
{
 InstructionLivenessInfo liveInfo = computeLiveInfo();
 map<shared_ptr<Symbol>, vector<shared_ptr<Symbol>>>
   interferenceGraph = constructInterferenceGraph(liveInfo);
 RegisterAllocationInfo spillInfo = determineRegisterAllocationOrder(interferenceGraph, 7);
 registerAssignment = allocateRegisters(spillInfo, interferenceGraph, 7);
}
