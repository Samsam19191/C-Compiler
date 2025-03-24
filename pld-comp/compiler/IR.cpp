#include "IR.h"
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <map>

using namespace std;
using std::runtime_error;
using std::to_string;

string target_arch = "x86"; // Cible par défaut

string ACC_REG = "%eax";
string BP_REG = "%rbp";
string RDI_REG = "%rdi";

/* ========================
   Implémentation de IRInstr
   ======================== */
IRInstr::IRInstr(BasicBlock *bb_, Operation op, Type t, vector<string> params)
    : bb(bb_), op(op), t(t), params(params)
{
}

void IRInstr::gen_asm(ostream &o)
{
    // Choix du code à générer selon la cible
    if (target_arch == "x86")
    {
        o << "    # x86 IRInstr: ";
    }
    else if (target_arch == "ARM")
    {
        o << "    // ARM IRInstr: ";
    }
    else if (target_arch == "MSP430")
    {
        o << "    ; MSP430 IRInstr: ";
    }
    else
    {
        throw runtime_error("Architecture cible inconnue: " + target_arch);
    }

    // On affiche le nom de l'opération pour le debug (optionnel)
    switch (op)
    {
    case ldconst:
        o << "ldconst";
        break;
    case copy:
        o << "copy";
        break;
    case add:
        o << "add";
        break;
    case sub:
        o << "sub";
        break;
    case mul:
        o << "mul";
        break;
    case rmem:
        o << "rmem";
        break;
    case wmem:
        o << "wmem";
        break;
    case call:
        o << "call";
        break;
    case cmp_eq:
        o << "cmp_eq";
        break;
    case cmp_lt:
        o << "cmp_lt";
        break;
    case cmp_le:
        o << "cmp_le";
        break;
    case mov:
        o << "mov";
        break;
    default:
        o << "unknown";
        break;
    }
    for (const auto &p : params)
    {
        o << " " << p;
    }

    // Traduction en code assembleur pour la cible x86 (exemple)
    if (target_arch == "x86")
    {
        switch (op)
        {
        case ldconst:
            // params: [dest, constant]
            o << "\n    movl $" << params[1] << ", " << params[0] << "\n";
            break;
        case copy:
            // params: [dest, src]
            o << "\n    movl " << params[1] << ", " << params[0] << "\n";
            break;
        case add:
            // params: [dest, src1, src2]
            o << "\n    movl " << params[1] << ", " << params[0] << "\n";
            o << "    addl " << params[2] << ", " << params[0] << "\n";
            break;
        case sub:
            o << "\n    movl " << params[1] << ", " << params[0] << "\n";
            o << "    subl " << params[2] << ", " << params[0] << "\n";
            break;
        case mul:
            o << "\n    movl " << params[1] << ", " << params[0] << "\n";
            o << "    imull " << params[2] << ", " << params[0] << "\n";
            break;
        case call:
            // Pour un appel, on émet simplement l'instruction call
            o << "\n    call " << params[0] << "\n";
            break;
        default:
            o << "\n    # Opération non implémentée\n";
            break;
        }
    }
    // Pour ARM ou MSP430, vous adapterez ici les instructions.

    o << "\n";
}

/* ========================
   Implémentation de BasicBlock
   ======================== */
BasicBlock::BasicBlock(CFG *cfg, string entry_label)
    : cfg(cfg), label(entry_label), exit_true(nullptr), exit_false(nullptr), test_var_name("")
{
}

void BasicBlock::add_IRInstr(IRInstr::Operation op, Type t, vector<string> params)
{
    IRInstr *instr = new IRInstr(this, op, t, params);
    instrs.push_back(instr);
}

void BasicBlock::gen_asm(ostream &o)
{
    // Affiche le label du bloc
    o << label << ":\n";

    // Génère l'assembleur pour chaque instruction IR dans le bloc
    for (auto instr : instrs)
    {
        instr->gen_asm(o);
    }

    // Gestion du branchement en fin de bloc
    if (exit_true == nullptr && exit_false == nullptr)
    {
        o << "    # Fin de bloc (terminal)\n";
    }
    else if (exit_false == nullptr)
    {
        o << "    jmp " << exit_true->label << "\n";
    }
    else
    {
        if (target_arch == "x86")
        {
            o << "    cmp " << test_var_name << ", $1\n";
            o << "    jne " << exit_false->label << "\n";
            o << "    jmp " << exit_true->label << "\n";
        }
        else if (target_arch == "ARM")
        {
            o << "    cmp " << test_var_name << ", #1\n";
            o << "    bne " << exit_false->label << "\n";
            o << "    b " << exit_true->label << "\n";
        }
        else if (target_arch == "MSP430")
        {
            o << "    cmp #" << 1 << ", " << test_var_name << "\n";
            o << "    jne " << exit_false->label << "\n";
            o << "    jmp " << exit_true->label << "\n";
        }
    }
}

/* ========================
   Implémentation de CFG
   ======================== */
CFG::CFG(DefFonction *ast)
    : ast(ast), nextFreeSymbolIndex(0), nextBBnumber(0), current_bb(nullptr)
{
}

void CFG::add_bb(BasicBlock *bb)
{
    bbs.push_back(bb);
    current_bb = bb;
}

string CFG::IR_reg_to_asm(string reg)
{
    if (target_arch == "x86")
    {
        int index = get_var_index(reg);
        return "-" + to_string(index) + "(%rbp)";
    }
    else
    {
        return reg;
    }
}

void CFG::gen_asm_prologue(ostream &o)
{
    if (target_arch == "x86")
    {
        o << ".att_syntax prefix\n";
        o << ".globl main\n";
        o << "main:\n";
        o << "    pushq %rbp\n";
        o << "    movq %rsp, %rbp\n";
    }
    else if (target_arch == "ARM")
    {
        o << ".global main\n";
        o << "main:\n";
        o << "    stmfd sp!, {fp, lr}\n";
        o << "    add fp, sp, #4\n";
    }
    else if (target_arch == "MSP430")
    {
        o << "    ; MSP430 prologue (à adapter)\n";
    }
}

void CFG::gen_asm_epilogue(ostream &o)
{
    if (target_arch == "x86")
    {
        o << "    popq %rbp\n";
        o << "    ret\n";
    }
    else if (target_arch == "ARM")
    {
        o << "    ldmfd sp!, {fp, lr}\n";
        o << "    bx lr\n";
    }
    else if (target_arch == "MSP430")
    {
        o << "    ; MSP430 épilogue (à adapter)\n";
    }
}

void CFG::gen_asm(ostream &o)
{
    gen_asm_prologue(o);
    for (auto bb : bbs)
    {
        bb->gen_asm(o);
    }
    gen_asm_epilogue(o);
}

void CFG::add_to_symbol_table(string name, Type t)
{
    SymbolType[name] = t;
    SymbolIndex[name] = nextFreeSymbolIndex;
    nextFreeSymbolIndex += 4;
}

string CFG::create_new_tempvar(Type t)
{
    int offset = nextFreeSymbolIndex;
    nextFreeSymbolIndex += 4;
    return "-" + to_string(offset) + "(%rbp)";
}

int CFG::get_var_index(string name)
{
    if (SymbolIndex.find(name) != SymbolIndex.end())
        return SymbolIndex[name];
    throw runtime_error("Variable non trouvée dans la table des symboles : " + name);
}

Type CFG::get_var_type(string name)
{
    if (SymbolType.find(name) != SymbolType.end())
        return SymbolType[name];
    throw runtime_error("Type non trouvé pour la variable : " + name);
}

string CFG::new_BB_name()
{
    return "BB_" + to_string(nextBBnumber++);
}
