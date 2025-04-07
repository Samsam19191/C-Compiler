#ifndef IR_H
#define IR_H

#include <vector>
#include <string>
#include <iostream>
#include <initializer_list>
#include <unordered_map>
#include <map>
#include "SymbolTableVisitor.h" // Votre gestion des symboles
#include "Type.h"               // Définition de Type
#include "DefFonction.h"        // Définition complète de DefFonction

// Pas de forward declaration de DefFonction ici, puisque nous l'incluons déjà.

class BasicBlock;
class CFG;

//! La classe pour une instruction 3-adresses
class IRInstr
{
public:
    typedef enum
    {
        ldconst,
        copy,
        add,
        sub,
        mul,
        rmem,
        wmem,
        call,
        cmp_eq,
        cmp_lt,
        cmp_le
    } Operation;

    IRInstr(BasicBlock *bb_, Operation op, Type t, std::vector<std::string> params);

    void gen_asm(std::ostream &o);

private:
    BasicBlock *bb;
    Operation op;
    Type t;
    std::vector<std::string> params;
};

class BasicBlock
{
public:
    BasicBlock(CFG *cfg, std::string entry_label);
    void gen_asm(std::ostream &o);
    void add_IRInstr(IRInstr::Operation op, Type t, std::vector<std::string> params);

    BasicBlock *exit_true;
    BasicBlock *exit_false;
    std::string label;
    CFG *cfg;
    std::vector<IRInstr *> instrs;
    std::string test_var_name;
};

class CFG
{
public:
    // Constructeur prenant une fonction AST complète
    CFG(DefFonction *ast);

    // Copie la table des symboles depuis le SymbolTableVisitor
    void setSymbolTable(const std::unordered_map<std::string, int> &st);

    DefFonction *ast;

    void add_bb(BasicBlock *bb);
    std::string new_BB_name();
    BasicBlock *current_bb;

    std::string IR_reg_to_asm(std::string reg);
    void gen_asm(std::ostream &o);
    void gen_asm_prologue(std::ostream &o);
    void gen_asm_epilogue(std::ostream &o);

    // Gestion de la table des symboles
    void add_to_symbol_table(std::string name, Type t);
    std::string create_new_tempvar(Type t);
    int get_var_index(std::string name);
    Type get_var_type(std::string name);

protected:
    std::map<std::string, Type> SymbolType;
    std::map<std::string, int> SymbolIndex;
    int nextFreeSymbolIndex;
    int nextBBnumber;
    std::vector<BasicBlock *> bbs;
};

#endif
