#ifndef BASICBLOCK_H
#define BASICBLOCK_H

#include <vector>       
#include <memory>      
#include <string>       
#include <ostream>      
#include "IR.h"         
#include "Symbol.h"    
#include "CFG.h"    

// ========== Classe BasicBlock ==========
// Représente un bloc de base dans le CFG
class BasicBlock
{
public:
  BasicBlock(CFG *cfg, string entry_label);
  void gen_asm(ostream &o); // Génère l'assembleur du bloc

  shared_ptr<Symbol> add_IRInstr(IRInstr::Operation operation, Type type,
                                      vector<Parameter> parameters);

  BasicBlock *exit_true;       // Bloc suivant si condition vraie
  BasicBlock *exit_false;      // Bloc suivant si condition fausse (sinon jump inconditionnel)
  bool visited;                // Indique si ce bloc a déjà été généré (utile pour éviter les doublons)
  string label;           // Label du bloc (nom unique)
  CFG *cfg;                    // CFG auquel appartient ce bloc
  vector<IRInstr> instructions; // Liste des instructions IR dans ce bloc
  string test_var_name;   // Nom de la variable de test (pour if / while, etc.)
}; 

#endif

