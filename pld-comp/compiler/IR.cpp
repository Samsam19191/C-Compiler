#include "IR.h"
#include "Type.h"
#include <sstream>
#include <cassert>
#include <cctype>
#include <map>
#include <iostream>
#include <unordered_map>

using namespace std;

////////////////////
// IRInstr methods

IRInstr::IRInstr(BasicBlock *bb_, Operation op, Type t, vector<string> params)
    : bb(bb_), op(op), t(t), params(params)
{
}

void IRInstr::gen_asm(ostream &o)
{
    // Sélection selon la cible définie dans le CFG
    if (bb->cfg->target == TargetArch::X86)
    {
        switch (op)
        {
        case ldconst:
            // ldconst : paramètres [dest, constant]
            o << "    movl $" << params[1] << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        case copy:
            // copy : paramètres [dest, src]
            o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        case add:
            o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "    addl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
            o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        case sub:
            o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "    subl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
            o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        case mul:
            o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "    imull " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
            o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        case rmem:
            o << "    movl (" << bb->cfg->IR_reg_to_asm(params[1]) << "), %eax\n";
            o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        case wmem:
            o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "    movl %eax, (" << bb->cfg->IR_reg_to_asm(params[0]) << ")\n";
            break;
        case call:
        {
            int numArgs = params.size() - 2;
            for (int i = numArgs; i >= 1; i--)
            {
                o << "    pushl " << bb->cfg->IR_reg_to_asm(params[i + 1]) << "\n";
            }
            o << "    call " << params[0] << "\n";
            if (numArgs > 0)
                o << "    addl $" << (numArgs * 4) << ", %esp\n";
            if (!params[1].empty())
                o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            break;
        }
        case cmp_eq:
            o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "    cmpl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
            o << "    sete %al\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        case cmp_lt:
            o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "    cmpl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
            o << "    setl %al\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        case cmp_le:
            o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "    cmpl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
            o << "    setle %al\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        default:
            o << "    # Opération IR non gérée\n";
            break;
        }
    }
    else if (bb->cfg->target == TargetArch::ARM64)
    {
        // Génération pour ARM64. Remarque : ce code est simplifié et peut être adapté.
        // Nous utiliserons par convention les registres x0, x1, x2, etc.
        switch (op)
        {
        case ldconst:
            // ldconst : paramètres [dest, constant]
            // Exemple : "mov x0, #42"
            o << "    mov x0, #" << params[1] << "\n";
            // Stocker x0 dans la variable destination
            o << "    str x0, [" << bb->cfg->IR_reg_to_asm(params[0]) << "]\n";
            break;
        case copy:
            // copy : paramètres [dest, src]
            o << "    ldr x0, [" << bb->cfg->IR_reg_to_asm(params[1]) << "]\n";
            o << "    str x0, [" << bb->cfg->IR_reg_to_asm(params[0]) << "]\n";
            break;
        case add:
            // add : paramètres [dest, op1, op2]
            o << "    ldr x1, [" << bb->cfg->IR_reg_to_asm(params[1]) << "]\n";
            o << "    ldr x2, [" << bb->cfg->IR_reg_to_asm(params[2]) << "]\n";
            o << "    add x0, x1, x2\n";
            o << "    str x0, [" << bb->cfg->IR_reg_to_asm(params[0]) << "]\n";
            break;
        case sub:
            o << "    ldr x1, [" << bb->cfg->IR_reg_to_asm(params[1]) << "]\n";
            o << "    ldr x2, [" << bb->cfg->IR_reg_to_asm(params[2]) << "]\n";
            o << "    sub x0, x1, x2\n";
            o << "    str x0, [" << bb->cfg->IR_reg_to_asm(params[0]) << "]\n";
            break;
        case mul:
            o << "    ldr x1, [" << bb->cfg->IR_reg_to_asm(params[1]) << "]\n";
            o << "    ldr x2, [" << bb->cfg->IR_reg_to_asm(params[2]) << "]\n";
            o << "    mul x0, x1, x2\n";
            o << "    str x0, [" << bb->cfg->IR_reg_to_asm(params[0]) << "]\n";
            break;
        case rmem:
            o << "    ldr x0, [" << bb->cfg->IR_reg_to_asm(params[1]) << "]\n";
            o << "    str x0, [" << bb->cfg->IR_reg_to_asm(params[0]) << "]\n";
            break;
        case wmem:
            o << "    ldr x0, [" << bb->cfg->IR_reg_to_asm(params[1]) << "]\n";
            o << "    str x0, [" << bb->cfg->IR_reg_to_asm(params[0]) << "]\n";
            break;
        case call:
        {
            // ARM64 utilise l'instruction BL pour appeler une fonction.
            // Ici, nous supposons que les arguments sont passés dans les registres x0-x7.
            // Pour les arguments supplémentaires, il faudrait gérer la pile.
            int numArgs = params.size() - 2;
            // On copie ici les arguments dans les registres correspondants :
            for (int i = 0; i < numArgs && i < 8; i++)
            {
                o << "    ldr x" << i << ", [" << bb->cfg->IR_reg_to_asm(params[i + 2]) << "]\n";
            }
            o << "    bl " << params[0] << "\n";
            // Stockage du résultat dans la destination, supposant qu'il est dans x0
            if (!params[1].empty())
                o << "    str x0, [" << bb->cfg->IR_reg_to_asm(params[1]) << "]\n";
            break;
        }
        case cmp_eq:
            // Comparaison pour égalité
            o << "    ldr x1, [" << bb->cfg->IR_reg_to_asm(params[1]) << "]\n";
            o << "    ldr x2, [" << bb->cfg->IR_reg_to_asm(params[2]) << "]\n";
            o << "    cmp x1, x2\n";
            o << "    cset w0, eq\n";
            o << "    str w0, [" << bb->cfg->IR_reg_to_asm(params[0]) << "]\n";
            break;
        case cmp_lt:
            o << "    ldr x1, [" << bb->cfg->IR_reg_to_asm(params[1]) << "]\n";
            o << "    ldr x2, [" << bb->cfg->IR_reg_to_asm(params[2]) << "]\n";
            o << "    cmp x1, x2\n";
            o << "    cset w0, lt\n";
            o << "    str w0, [" << bb->cfg->IR_reg_to_asm(params[0]) << "]\n";
            break;
        case cmp_le:
            o << "    ldr x1, [" << bb->cfg->IR_reg_to_asm(params[1]) << "]\n";
            o << "    ldr x2, [" << bb->cfg->IR_reg_to_asm(params[2]) << "]\n";
            o << "    cmp x1, x2\n";
            o << "    cset w0, le\n";
            o << "    str w0, [" << bb->cfg->IR_reg_to_asm(params[0]) << "]\n";
            break;
        default:
            o << "    // Opération IR non gérée pour ARM64\n";
            break;
        }
    }
}

////////////////////
// BasicBlock methods

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
    o << label << ":\n";
    for (IRInstr *instr : instrs)
    {
        instr->gen_asm(o);
    }
    if (exit_true == nullptr)
    {
        cfg->gen_asm_epilogue(o);
    }
    else if (exit_false == nullptr)
    {
        o << "    jmp " << exit_true->label << "\n";
    }
    else
    {
        o << "    cmp x0, #0\n"; // Pour ARM64, exemple de comparaison
        o << "    beq " << exit_false->label << "\n";
        o << "    b " << exit_true->label << "\n";
    }
}

////////////////////
// CFG methods

CFG::CFG(DefFonction *ast)
    : ast(ast), nextFreeSymbolIndex(0), nextBBnumber(0), current_bb(nullptr)
{
    // Par défaut, on choisit la cible x86
    target = TargetArch::X86;
}

void CFG::add_bb(BasicBlock *bb)
{
    bbs.push_back(bb);
}

string CFG::new_BB_name()
{
    ostringstream oss;
    oss << "BB" << nextBBnumber++;
    return oss.str();
}

string CFG::IR_reg_to_asm(string reg)
{
    if (target == TargetArch::X86)
    {
        // Pour x86 : si 'reg' est une constante numérique, renvoie "$<constante>"
        if (!reg.empty() && (isdigit(reg[0]) || (reg[0] == '-' && reg.size() > 1 && isdigit(reg[1]))))
        {
            return "$" + reg;
        }
        auto it = SymbolIndex.find(reg);
        if (it != SymbolIndex.end())
        {
            ostringstream oss;
            oss << it->second << "(%rbp)";
            return oss.str();
        }
        else
        {
            cerr << "Erreur : variable '" << reg << "' introuvable dans la table des symboles.\n";
            return reg;
        }
    }
    else if (target == TargetArch::ARM64)
    {
        // Pour ARM64, on suppose que les variables sont stockées par rapport au frame pointer (x29)
        // Exemple : on retourne "[x29, #-offset]"
        if (!reg.empty() && (isdigit(reg[0]) || (reg[0] == '-' && reg.size() > 1 && isdigit(reg[1]))))
        {
            return "#" + reg;
        }
        auto it = SymbolIndex.find(reg);
        if (it != SymbolIndex.end())
        {
            ostringstream oss;
            oss << "[x29, #(" << it->second << ")]";
            return oss.str();
        }
        else
        {
            cerr << "Erreur : variable '" << reg << "' introuvable dans la table des symboles.\n";
            return reg;
        }
    }
    return "";
}

void CFG::gen_asm(ostream &o)
{
    gen_asm_prologue(o);
    for (BasicBlock *bb : bbs)
    {
        bb->gen_asm(o);
    }
}

void CFG::gen_asm_prologue(ostream &o)
{
    if (target == TargetArch::X86)
    {
        o << "    .globl " << ast->getName() << "\n";
        o << ast->getName() << ":\n";
        o << "    pushq %rbp\n";
        o << "    movq %rsp, %rbp\n";
    }
    else if (target == TargetArch::ARM64)
    {
        // Pour ARM64, un prologue simplifié :
        o << "    .global " << ast->getName() << "\n";
        o << ast->getName() << ":\n";
        o << "    stp x29, x30, [sp, #-16]!\n"; // sauvegarde du FP et LR
        o << "    mov x29, sp\n";
    }
}

void CFG::gen_asm_epilogue(ostream &o)
{
    if (target == TargetArch::X86)
    {
        o << "    popq %rbp\n";
        o << "    ret\n";
    }
    else if (target == TargetArch::ARM64)
    {
        // Épilogue simplifié pour ARM64 :
        o << "    ldp x29, x30, [sp], #16\n";
        o << "    ret\n";
    }
}

void CFG::setSymbolTable(const unordered_map<string, int> &st)
{
    SymbolIndex.clear();
    for (const auto &p : st)
    {
        SymbolIndex[p.first] = p.second;
    }
}

void CFG::add_to_symbol_table(std::string name, Type t)
{
    SymbolType[name] = t;
    if (SymbolIndex.find(name) == SymbolIndex.end())
    {
        nextFreeSymbolIndex++;
        SymbolIndex[name] = -(nextFreeSymbolIndex * 4);
    }
}

string CFG::create_new_tempvar(Type t)
{
    ostringstream oss;
    oss << "t" << nextFreeSymbolIndex++;
    string temp = oss.str();
    SymbolIndex[temp] = -(nextFreeSymbolIndex * 4);
    return temp;
}

int CFG::get_var_index(string name)
{
    auto it = SymbolIndex.find(name);
    if (it != SymbolIndex.end())
        return it->second;
    else
    {
        cerr << "Erreur : variable '" << name << "' introuvable dans la table des symboles.\n";
        return 0;
    }
}

Type CFG::get_var_type(string name)
{
    return SymbolType.find(name) != SymbolType.end() ? SymbolType[name] : Type::INT;
}
