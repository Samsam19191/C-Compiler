#include "IR.h"
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <map>


using std::vector;
using std::string;
using std::ostream;
using std::map;

// Définition de la variable globale target_arch
// Vous pouvez changer cette valeur pour "ARM" ou "MSP430" selon la cible souhaitée.
std::string target_arch = "x86";


/* ========================
   Implémentation de IRInstr
   ======================== */
IRInstr::IRInstr(BasicBlock* bb_, Operation op, Type t, vector<string> params)
    : bb(bb_), op(op), t(t), params(params) {
}

void IRInstr::gen_asm(ostream &o) {
    // Choix du code à générer selon l'architecture cible
    if (target_arch == "x86") {
        o << "    # x86 IRInstr: ";
        
    } else if (target_arch == "ARM") {
        o << "    // ARM IRInstr: ";
    } else if (target_arch == "MSP430") {
        o << "    ; MSP430 IRInstr: ";
    } else {
        throw std::runtime_error("Architecture cible inconnue: " + target_arch);
    }
    
    // Génération simplifiée : afficher l'opération et ses paramètres.
    switch(op) {
        case ldconst: o << "ldconst"; break;
        case copy:    o << "copy";    break;
        case add:     o << "add";     break;
        case sub:     o << "sub";     break;
        case mul:     o << "mul";     break;
        case rmem:    o << "rmem";    break;
        case wmem:    o << "wmem";    break;
        case call:    o << "call";    break;
        case cmp_eq:  o << "cmp_eq";  break;
        case cmp_lt:  o << "cmp_lt";  break;
        case cmp_le:  o << "cmp_le";  break;
        default:      o << "unknown"; break;
    }
    for (const auto &p : params) {
        o << " " << p;
    }

    switch(op) {
            case ldconst:
                // params: [dest, constant]
                o << "    movl $" << params[1] << ", " << params[0] << "\n";
                break;
            case copy:
                // params: [dest, src]
                o << "    movl " << params[1] << ", " << params[0] << "\n";
                break;
            case add:
                // params: [dest, src1, src2]
                // On suppose ici que le résultat doit être placé dans dest.
                o << "    movl " << params[1] << ", " << params[0] << "\n";
                o << "    addl " << params[2] << ", " << params[0] << "\n";
                break;
            case sub:
                o << "    movl " << params[1] << ", " << params[0] << "\n";
                o << "    subl " << params[2] << ", " << params[0] << "\n";
                break;
            case mul:
                o << "    movl " << params[1] << ", " << params[0] << "\n";
                o << "    imull " << params[2] << ", " << params[0] << "\n";
                break;
            // Vous pouvez ajouter d'autres opérations de la même manière.
            default:
                o << "    # Opération non implémentée\n";
                break;
        }
    
    o << "\n";
}


/* ========================
   Implémentation de BasicBlock
   ======================== */
BasicBlock::BasicBlock(CFG* cfg, string entry_label)
    : cfg(cfg), label(entry_label), exit_true(nullptr), exit_false(nullptr), test_var_name("") {
}

void BasicBlock::add_IRInstr(IRInstr::Operation op, Type t, vector<string> params) {
    IRInstr* instr = new IRInstr(this, op, t, params);
    instrs.push_back(instr);
}

void BasicBlock::gen_asm(ostream &o) {
    // Affichage du label du bloc
    o << label << ":\n";
    
    // Génération de l'assembleur pour chaque instruction IR dans le bloc
    for (auto instr : instrs) {
        instr->gen_asm(o);
    }
    
    // Gestion du branchement en fin de bloc
    if (exit_true == nullptr && exit_false == nullptr) {
        // Bloc terminal (aucun saut à générer)
        o << "    # Fin de bloc (terminal)\n";
    } else if (exit_false == nullptr) {
        // Saut inconditionnel vers exit_true
        o << "    jmp " << exit_true->label << "\n";
    } else {
        // Branchage conditionnel : on utilise test_var_name pour comparer le résultat du test
        if (target_arch == "x86") {
            o << "    cmp " << test_var_name << ", $1\n";
            o << "    jne " << exit_false->label << "\n";
            o << "    jmp " << exit_true->label << "\n";
        } else if (target_arch == "ARM") {
            o << "    cmp " << test_var_name << ", #1\n";
            o << "    bne " << exit_false->label << "\n";
            o << "    b " << exit_true->label << "\n";
        } else if (target_arch == "MSP430") {
            o << "    cmp #" << 1 << ", " << test_var_name << "\n";
            o << "    jne " << exit_false->label << "\n";
            o << "    jmp " << exit_true->label << "\n";
        }
    }
}


/* ========================
   Implémentation de CFG
   ======================== */
CFG::CFG(DefFonction* ast)
    : ast(ast), nextFreeSymbolIndex(0), nextBBnumber(0), current_bb(nullptr) {
}

void CFG::add_bb(BasicBlock* bb) {
    bbs.push_back(bb);
    current_bb = bb;
}

string CFG::IR_reg_to_asm(string reg) {
    // Conversion simplifiée : retourne l'offset sous forme "-offset(%rbp)" pour x86,
    // ou une version adaptée pour ARM/MSP430 (ici on renvoie le nom tel quel pour simplifier)
    if (target_arch == "x86") {
        int index = get_var_index(reg);
        return "-" + std::to_string(index) + "(%rbp)";
    } else {
        return reg; // Pour ARM/MSP430, adapter selon vos conventions
    }
}

void CFG::gen_asm_prologue(ostream &o) {
    if (target_arch == "x86") {
        // Ajout de la directive globale et du label main
        o << ".globl main\n";
        o << "main:\n";
        o << "    pushq %rbp\n";
        o << "    movq %rsp, %rbp\n";
    } else if (target_arch == "ARM") {
        o << ".global main\n";
        o << "main:\n";
        o << "    stmfd sp!, {fp, lr}\n";
        o << "    add fp, sp, #4\n";
    } else if (target_arch == "MSP430") {
        o << "    ; MSP430\n";
    }
}


void CFG::gen_asm_epilogue(ostream &o) {
    if (target_arch == "x86") {
        o << "    popq %rbp\n";
        o << "    ret\n";
    } else if (target_arch == "ARM") {
        o << "    ldmfd sp!, {fp, lr}\n";
        o << "    bx lr\n";
    } else if (target_arch == "MSP430") {
        o << "    ; MSP430 épilogue (à adapter)\n";
    }
}

void CFG::gen_asm(ostream &o) {
    // Génère le prologue
    gen_asm_prologue(o);
    
    // Génère l'assembleur pour chaque BasicBlock
    for (auto bb : bbs) {
        bb->gen_asm(o);
    }
    
    // Génère l'épilogue
    gen_asm_epilogue(o);
}

void CFG::add_to_symbol_table(string name, Type t) {
    SymbolType[name] = t;
    // Par exemple, on alloue 4 octets par variable pour x86
    SymbolIndex[name] = nextFreeSymbolIndex;
    nextFreeSymbolIndex += 4;
}

string CFG::create_new_tempvar(Type t) {
    // Crée un nom temporaire unique avec un préfixe "!" pour éviter les conflits avec les variables C
    string temp = "!tmp" + std::to_string(nextFreeSymbolIndex / 4);
    add_to_symbol_table(temp, t);
    return temp;
}

int CFG::get_var_index(string name) {
    if (SymbolIndex.find(name) != SymbolIndex.end())
        return SymbolIndex[name];
    throw std::runtime_error("Variable non trouvée dans la table des symboles : " + name);
}

Type CFG::get_var_type(string name) {
    if (SymbolType.find(name) != SymbolType.end())
        return SymbolType[name];
    throw std::runtime_error("Type non trouvé pour la variable : " + name);
}

string CFG::new_BB_name() {
    return "BB_" + std::to_string(nextBBnumber++);
}
